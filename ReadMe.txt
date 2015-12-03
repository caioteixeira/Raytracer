Assignment #3: Ray tracing

FULL NAME: Caio Vinicius Marques Teixeira

Note: I am using the late day policy. I submitted the homework two day after the due date.


Feature:                                 Status: finish? (yes/no)

-------------------------------------    -------------------------

1) Ray tracing triangles                 yes

2) Ray tracing sphere                    yes

3) Triangle Phong Shading                yes

4) Sphere Phong Shading                  yes

5) Shadows rays                          yes

6) Still images                          yes
   
7) Extra Credit (up to 20 points)

- Fast triangle intersection: I implemented the Möller–Trumbore ray-triangle intersection algorithm as a faster method to calculate the intersection of a ray and a triangle.
- Multithreaded raytracing: I implemented a multithread implementation to improve the raytracing performance, so each thread trace an entire row.
You can adjust how many threads will work in the rendering by modifying the  N_THREADS variable.
- Supersampling Antialiasing: My solution to the aliasing problem is a implementation of super sampling. 
You can change the supersampling level by modifying the SAMPLING_FACTOR variable.
I included some stills images to show the difference between each sampling level.
- Recursive reflection: I implemented a simple recursive solution to handle reflections.
To turn on reflection, modify the variable doReflection.
To change max level of recursions, just modify the MAX_RECURSIONS variable.
