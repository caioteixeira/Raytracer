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
#include <queue>

#include <stdio.h>
#include <string>
#include <thread>
#include <limits>

#include <iostream>

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename = 0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode = MODE_DISPLAY;

/*Multithreading config*/
int N_THREADS = 8; //Number of multiple threads running
int ACTIVE_THREADS = 0;
std::queue<std::thread *> threads;

//Reflection?
bool doReflection = false;
int MAX_RECURSIONS = 5; /*Max ecursion level for reflection*/

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480 
//image plane size
double PLANE_HEIGHT;
double PLANE_WIDTH;

double SAMPLING_FACTOR = 3.0f;

//the field of view of the camera
#define fov 60.0

//Pi Value
const double PI = 3.1459265;

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
//returns the distance in the x axis
double distance(Ray a, Light b);
Vector reflection(Vector direction, Vector normal);

Vector buildVector(double ar[3]);

Vector posAtRay(Ray r, double t);

Vector computePhongShading(Vector intersection, Vector normal, Vector kd, Vector ks, double sh, Vector v);

/*Intersections*/
bool intersectsLight(Ray ray, Light light);
bool intersectsSphere(Ray ray, Sphere sphere, double & distance);
bool intersectsTriangle(Ray ray, Triangle tri, double & distance);


void traceRow(int i, double y);
Vector trace(Ray r);

void waitThreads(bool mid = false);


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
	double a = (double) WIDTH / (double) HEIGHT;


	double x = a * tan(fov / 2 * (PI / 180)) ;
	double y = tan(fov / 2 * (PI / 180));
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
			color.x = 0.0;
			color.y = 0.0;
			color.z = 0.0;
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
			
		if (ACTIVE_THREADS >= N_THREADS) {
			waitThreads(true);
		}

		//std::cout << "Starting Row: " << i << std::endl;

		std::thread * t = new std::thread(traceRow , i, y);
		threads.push(t);
		ACTIVE_THREADS++;
		y += PLANE_HEIGHT / (HEIGHT*SAMPLING_FACTOR);
	}

	waitThreads();
}

void traceRow(int j, double y) {

	double x = bottomLeft.x;
	for (int i = 0; i < WIDTH * SAMPLING_FACTOR; i++) {

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

		pixels[j][i].x = color.x;
		pixels[j][i].y = color.y;
		pixels[j][i].z = color.z;

		x += PLANE_WIDTH / (WIDTH*SAMPLING_FACTOR);
	}

	//std::cout << "Ending Row: " << j << std::endl;
}

Vector trace(Ray r) {
	Vector c = Vector();
	c.x = 0.0;
	c.y = 0.0;
	c.z = 0.0;

	bool intersects = false;

	double furthestDist = DBL_MAX; //Maximum double value

	//Light intersections
	for (int i = 0; i < num_lights; i++){
		if (intersectsLight(r, lights[i])){
			double dist = distance(r, lights[i]);
			if (dist < furthestDist){
				furthestDist = dist;
				intersects = true;
				
				c.x = lights[i].color[0] * 255.0;
				c.y = lights[i].color[1] * 255.0;
				c.z = lights[i].color[2] * 255.0;
			}
		}
	}

	//Triangle intersections
	for (int i = 0; i < num_triangles; i++){
		double triangleDist = DBL_MAX;
		if (intersectsTriangle(r, triangles[i], triangleDist)){
			if (triangleDist < furthestDist){
				furthestDist = triangleDist;
				intersects = true;

				c.x = 0.0;
				c.y = 0.0;
				c.z = 0.0;
			}
		}
	}



	//Sphere intersections
	for (int i = 0; i < num_spheres; i++){
		double dist;
		if (intersectsSphere(r, spheres[i], dist)){
			if (dist < furthestDist){
				furthestDist = dist;
				intersects = true;

				/*Calculate normal*/
				Vector intersection = posAtRay(r, dist);
				Vector normal;
				normal.x = intersection.x - spheres[i].position[0];
				normal.y = intersection.y - spheres[i].position[1];
				normal.z = intersection.z - spheres[i].position[2];
				normal = normalize(normal);


				/*Phong model variables*/
				double sh = spheres[i].shininess;
				Vector kd;
				kd.x = spheres[i].color_diffuse[0];
				kd.y = spheres[i].color_diffuse[1];
				kd.z = spheres[i].color_diffuse[2];
				Vector ks;
				ks.x = spheres[i].color_specular[0];
				ks.y = spheres[i].color_specular[1];
				ks.z = spheres[i].color_specular[2];
				Vector v = scalarMultiply(-1.0, r.direction);
				v = normalize(v);

				Vector phongColor = computePhongShading(intersection, normal, kd, ks, sh, v);

				c.x = phongColor.x * 255.0;
				c.y = phongColor.y * 255.0;
				c.z = phongColor.z * 255.0;

			}
		}
	}



	if (intersects){
		c.x += ambient_light[0] * 255.0;
		c.y += ambient_light[1] * 255.0;
		c.z += ambient_light[2] * 255.0;
	}
	else {
		c.x = 255.0;
		c.y = 255.0;
		c.z = 255.0;
	}

	c.x = max(min(c.x, 255.0), 0.0);
	c.y = max(min(c.y, 255.0), 0.0);
	c.z = max(min(c.z, 255.0), 0.0);

	return c;
}

bool intersectsLight(Ray ray, Light light){
	/*
	p(t) = p0 + dt;
	*/

	//if camera and light has the same position
	if (light.position[0] == ray.origin.x && light.position[1] == ray.origin.y && light.position[2] == ray.origin.z){
		return false;
	}

	double dist = (light.position[0] - ray.origin.x) / ray.direction.x;
	double distY = (light.position[1] - ray.origin.y) / ray.direction.y;
	double distZ = (light.position[2] - ray.origin.z) / ray.direction.z;

	if (dist != distY || dist != distZ){
		return false;
	}

	return true;
}

bool intersectsSphere(Ray ray, Sphere sphere, double & distance){
	/*Reference: http://www.ccs.neu.edu/home/fell/CSU540/programs/RayTracingFormulas.htm */

	double radius = sphere.radius;
	
	double b = 2.0 * (
		ray.direction.x * (ray.origin.x - sphere.position[0]) + 
		ray.direction.y * (ray.origin.y - sphere.position[1]) + 
		ray.direction.z * (ray.origin.z - sphere.position[2]));

	double c = pow(ray.origin.x - sphere.position[0], 2.0) + 
		pow(ray.origin.y - sphere.position[1], 2.0) + 
		pow(ray.origin.z - sphere.position[2], 2.0) - 
		pow(radius, 2.0);

	double delta = pow(b, 2.0) - 4.0 * c;

	/*No intersections*/
	if (delta < 0.0) 
		return false;

	double t0 = (-b + sqrt(delta)) / 2;
	double t1 = (-b - sqrt(delta)) / 2; 

	if (delta == 0.0) //ray is tanget to the sphere
		distance = max(t0, t1);
	else
		distance = min(t0, t1);

	if (distance < 0.001) //Workaround for phong model
		return false;

	return true;
}
bool intersectsTriangle(Ray ray, Triangle tri, double & dist){
	/*Möller–Trumbore intersection algorithm
		thanks wikipedia :)
		https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	*/
	double e = 0.000001;

	Vector v1 = buildVector(tri.v[0].position);
	Vector v2 = buildVector(tri.v[1].position);
	Vector v3 = buildVector(tri.v[2].position);

	Vector e1 = minusVectors(v1, v2);
	Vector e2 = minusVectors(v3, v1);
	
	//Begin calculating determinant - also used to calculate u parameter
	Vector p = crossProduct(ray.direction, e2);
	double det = dotProduct(e1, p);

	//If determinant is near zero, ray lies in plane of triangle
	if (det > -e && det < e) 
		return false;
	double inv_det = 1.0 / det;

	Vector t = minusVectors(ray.origin, v1);
	double u = dotProduct(t, p) * inv_det;
	//The intersection lies outside of the triangle
	if (u < 0.0 || u > 1.0)
		return false;

	Vector q = crossProduct(t, e1);
	double v = dotProduct(ray.direction, q) * inv_det;
	//The intersection lies outside of the triangle
	if (v < 0.0 || u + v > 1.0)
		return false;

	double a = dotProduct(e2, q) * inv_det;

	if (a > e){
		dist = a;
		return true;
	}

	return false;
}

Vector computePhongShading(Vector intersection, Vector normal, Vector kd, Vector ks, double sh, Vector v){
	Vector color;

	color.x = 0.0;
	color.y = 0.0;
	color.z = 0.0;

	//Iterate the lights
	for (int i = 0; i < num_lights; i++){
		bool shadowed = false;

		/*Create the shadow ray*/
		Vector lightPos;
		lightPos.x = lights[i].position[0]; lightPos.y = lights[i].position[1]; lightPos.z = lights[i].position[2];

		Vector direction = minusVectors(lightPos, intersection);
		direction = normalize(direction);

		Ray shadowRay;
		shadowRay.direction = direction;
		shadowRay.origin = intersection;

		double dist = distance(lightPos, intersection);

		/*Triangle intersection*/
		for (int j = 0; j < num_triangles; j++){
			double distTriangles;
			if (intersectsTriangle(shadowRay, triangles[j], distTriangles)){
				Vector sInter = posAtRay(shadowRay, distTriangles);
				distTriangles = distance(sInter, intersection);
				if (distTriangles <= dist){
					shadowed = true;
				}
			}
		}


		/*Sphere intersection*/
		for (int j = 0; j < num_spheres; j++){
			double distSphere;
			if (intersectsSphere(shadowRay, spheres[j], distSphere)){
				Vector sInter = posAtRay(shadowRay, distSphere);
				distSphere = distance(sInter, intersection);
				if (distSphere <= dist){
					shadowed = true;
				}
			}

		}

		if (!shadowed){
			//Phong model
			double LdN = dotProduct(direction, normal);
			if (LdN < 0.0)
				LdN = 0.0;

			Vector r = normalize(reflection(direction, normal));
			double RdV = dotProduct(r, v);
			if (RdV < 0.0) 
				RdV = 0.0;

			color.x += lights[i].color[0] * (kd.x * LdN + ks.x * pow(RdV, sh));
			color.y += lights[i].color[1] * (kd.y * LdN + ks.y * pow(RdV, sh));
			color.z += lights[i].color[2] * (kd.z * LdN + ks.z * pow(RdV, sh));
		}
	}

	return color;

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

double distance(Ray a, Light b){
	return (b.position[0] - a.origin.x) / a.direction.x;
}


Vector posAtRay(Ray r, double t){
	Vector pos;
	pos.x = r.origin.x + r.direction.x * t;
	pos.y = r.origin.y + r.direction.y * t;
	pos.z = r.origin.z + r.direction.z * t;
	return pos;
}

Vector reflection(Vector direction, Vector normal){
	double DdN = dotProduct(direction, normal);

	Vector r;
	r.x = 2.0 * DdN * normal.x - direction.x;
	r.y = 2.0 * DdN * normal.y - direction.y;
	r.z = 2.0 * DdN * normal.z - direction.z;
	return r;
}

Vector buildVector(double ar[3]){
	Vector r;
	r.x = ar[0];
	r.y = ar[1];
	r.z = ar[2];
	return r;
}


//Wait for all threads
//if mid is true, only wait for a half of the running threads
void waitThreads(bool mid) {

	int maxThreads = mid ? N_THREADS / 2 : 0;

	while (ACTIVE_THREADS >= maxThreads && !threads.empty()) {
		threads.front()->join();
		threads.pop();
		ACTIVE_THREADS--;
	}
}
