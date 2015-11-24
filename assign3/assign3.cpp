/*
CSCI 480
Assignment 3 Raytracer

Name: Caio Vinicius Marques Teixeira
*/

#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>

#include <stdio.h>
#include <string>

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename = 0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480
//image plane size
double PLANE_HEIGHT;
double PLANE_WIDTH;

double SAMPLING_FACTOR = 2.0f;

//the field of view of the camera
#define fov 60.0

//Pi Value
const double PI = 3.1459265;

//Reflection?
bool doReflection = false;

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vector {
	double x;
	double y;
	double z;
};

struct Vertex
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double normal[3];
	double shininess;
};

typedef struct _Triangle
{
	struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
	double radius;
} Sphere;

typedef struct _Light
{
	double position[3];
	double color[3];
} Light;

struct Ray {
	Vector origin;
	Vector direction;
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

/*Camera*/
Vector cameraPos;

/*Screen corners*/
Vector topLeft;
Vector topRight;
Vector bottomLeft;
Vector bottomRight;

//Store the pixels of the screen
std::vector<std::vector<Vector>> pixels;


/*Functions declarations*/
Vector trace(Ray r);
void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
Vector normalize(Vector p);
Vector crossProduct(Vector a, Vector b);
Vector addVectors(Vector a, Vector b);
Vector minusVectors(Vector a, Vector b);
Vector scalarMultiply(double s, Vector a);
double dotProduct(Vector a, Vector b);
double distance(Vector a, Vector b);


void render_scene();

//MODIFY THIS FUNCTION
void draw_scene()
{
	render_scene();

	unsigned int x, y;
	//simple output
	int col = 0;
	for (x = 0; x < WIDTH * SAMPLING_FACTOR; x+=SAMPLING_FACTOR)
	{
		int row = 0;
		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (y = 0; y < HEIGHT * SAMPLING_FACTOR; y+=SAMPLING_FACTOR)
		{
			double r = 0.0;
			double g = 0.0;
			double b = 0.0;

			/*Supersampling*/
			for (int i = 0; i < SAMPLING_FACTOR; i++) {
				for (int j = 0; j < SAMPLING_FACTOR; j++) {
					r += pixels[y + j][i + x].x;
					g += pixels[y + j][i + x].y;
					b += pixels[y + j][i + x].z;
				}
			}

			r /= pow(SAMPLING_FACTOR, 2);
			g /= pow(SAMPLING_FACTOR, 2);
			b /= pow(SAMPLING_FACTOR, 2);


			plot_pixel(col, row, r, g, b);
			row++;
		}

		col++;
		glEnd();
		glFlush();
	}
	printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	glColor3f(((double)r) / 256.f, ((double)g) / 256.f, ((double)b) / 256.f);
	glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	buffer[HEIGHT - y - 1][x][0] = r;
	buffer[HEIGHT - y - 1][x][1] = g;
	buffer[HEIGHT - y - 1][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	plot_pixel_display(x, y, r, g, b);
	if (mode == MODE_JPEG)
		plot_pixel_jpeg(x, y, r, g, b);
}

void save_jpg()
{
	Pic *in = NULL;

	in = pic_alloc(640, 480, 3, NULL);
	printf("Saving JPEG file: %s\n", filename);

	memcpy(in->pix, buffer, 3 * WIDTH*HEIGHT);
	if (jpeg_write(filename, in))
		printf("File saved Successfully\n");
	else
		printf("Error in Saving\n");

	pic_free(in);

}

void parse_check(char *expected, char *found)
{
	if (stricmp(expected, found))
	{
		char error[100];
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}

}

void parse_doubles(FILE*file, char *check, double p[3])
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE*file, double *r)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%lf", r);
	printf("rad: %f\n", *r);
}

void parse_shi(FILE*file, double *shi)
{
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%lf", shi);
	printf("shi: %f\n", *shi);
}

int loadScene(char *argv)
{
	FILE *file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	int i;
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);
	char str[200];

	parse_doubles(file, "amb:", ambient_light);

	for (i = 0; i < number_of_objects; i++)
	{
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (stricmp(type, "triangle") == 0)
		{

			printf("found triangle\n");
			int j;

			for (j = 0; j < 3; j++)
			{
				parse_doubles(file, "pos:", t.v[j].position);
				parse_doubles(file, "nor:", t.v[j].normal);
				parse_doubles(file, "dif:", t.v[j].color_diffuse);
				parse_doubles(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (num_triangles == MAX_TRIANGLES)
			{
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = t;
		}
		else if (stricmp(type, "sphere") == 0)
		{
			printf("found sphere\n");

			parse_doubles(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_doubles(file, "dif:", s.color_diffuse);
			parse_doubles(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (num_spheres == MAX_SPHERES)
			{
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = s;
		}
		else if (stricmp(type, "light") == 0)
		{
			printf("found light\n");
			parse_doubles(file, "pos:", l.position);
			parse_doubles(file, "col:", l.color);

			if (num_lights == MAX_LIGHTS)
			{
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights[num_lights++] = l;
		}
		else
		{
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	return 0;
}


void calculateBorders() {
	//aspect ratio
	double a = WIDTH / HEIGHT;


	double x = a * tan(fov / 2);
	double y = tan(fov / 2);
	double z = -1.0;

	topLeft.x = -1 * x;
	topLeft.y = y;
	topLeft.z = z;

	topRight.x = x;
	topRight.y = y;
	topRight.z = z;

	bottomLeft.x = -1 * x;
	bottomLeft.y = -1 * y;
	bottomLeft.z = z;

	bottomRight.x = x;
	bottomRight.y = -1 * y;
	bottomRight.z = z;

	PLANE_HEIGHT = 2.0 * y;
	PLANE_WIDTH = 2.0 * x;
}

//Init the screen pixels
void initScreen() {
	for (int i = 0; i < HEIGHT * SAMPLING_FACTOR; i++) {
		std::vector<Vector> row;
		for (int j = 0; j < WIDTH * SAMPLING_FACTOR; j++) {
			Vector color;
			color.x = 255.0;
			color.y = 255.0;
			color.z = 255.0;
			row.push_back(color);
		}
		pixels.push_back(row);
	}

	cameraPos.x = 0.0;
	cameraPos.y = 0.0;
	cameraPos.z = 0.0;
}

void render_scene() {

	double y = bottomLeft.y;
	for (int i = 0; i < HEIGHT * SAMPLING_FACTOR; i++) {
		double x = bottomLeft.x;
		for (int j = 0; j < WIDTH * SAMPLING_FACTOR; j++) {

			Vector pixel_position;
			pixel_position.x = x;
			pixel_position.y = y;
			pixel_position.z = -1.0;

			/*Ray direction*/
			Vector direction = minusVectors(pixel_position, cameraPos);
			direction = normalize(direction);

			/*Trace the ray*/
			Ray r;
			r.origin = cameraPos;
			r.direction = direction;
			Vector color = trace(r);

			pixels[i][j].x = color.x;
			pixels[i][j].y = color.y;
			pixels[i][j].z = color.z;

			x += PLANE_WIDTH / (WIDTH*SAMPLING_FACTOR);
		}
		y += PLANE_HEIGHT / (HEIGHT*SAMPLING_FACTOR);
	}
	
}

Vector trace(Ray r) {
	Vector c;
	

	return c;
}

void display()
{
	
}

void init()
{
	calculateBorders();
	initScreen();

	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
	//hack to make it only draw once
	static int once = 0;
	if (!once)
	{
		draw_scene();
		if (mode == MODE_JPEG)
			save_jpg();
	}
	once = 1;
}

int main(int argc, char ** argv)
{
	if (argc < 2 || argc > 3)
	{
		printf("usage: %s <scenefile> [jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 3)
	{
		mode = MODE_JPEG;
		filename = argv[2];
	}
	else if (argc == 2)
		mode = MODE_DISPLAY;

	glutInit(&argc, argv);
	loadScene(argv[1]);

	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	int window = glutCreateWindow("Ray Tracer");
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
}


/*Vector math helper functions*/
Vector normalize(Vector p) {
	double length = sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z, 2));
	Vector r;
	r.x = p.x / length;
	r.y = p.y / length;
	r.z = p.z / length;
	return r;
}
Vector crossProduct(Vector a, Vector b) {
	Vector r;
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
	return r;
}

Vector addVectors(Vector a, Vector b) {
	Vector r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
	return r;
}

Vector minusVectors(Vector a, Vector b) {
	Vector r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;
	return r;
}

Vector scalarMultiply(double s, Vector a) {
	Vector r;
	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;
	return r;
}

double dotProduct(Vector a, Vector b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

double distance(Vector a, Vector b) {
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
}
