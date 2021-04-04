// Author: Ahmed Nouralla

/*
 * GeneticArt project main source code file
 *
 * Quick testing:
 *    Just replace 'input.bmp' with another image of the same name (respecting the requirements below)
 *    then run "./a.out" from the terminal
 *
 * Requirements for the code to compile/run correctly and properly
 *   1. Environment: Any linux distribution with gcc compiler (CODE WILL NOT WORK ON WINDOWS)
 *   2. Dependencies: OpenGL Utility Toolkit (GLUT) installed ('GL/' has to exist in directory '/usr/include/')
 *   3. Input image: a valid 512x512 RGB bitmap (*.bmp) file (generated by software such as MS-Paint or Pinta)
 * Note: Performance is machine dependent (a modern machine implies faster generation)
 *
 * Compilation under linux (tested on Ubuntu 20.04 LTS)
 *   1. Specify the input image file location below (INPUT_IMAGE_PATH)
 *   2. Open terminal, navigate (cd) to the directory containing this file
 *   3. Run "sudo sh ./compile.sh" (without quotes) to compile the source files into an output executable
 *   4. Execute the generated file using "./a.out"
 *
*/

#define INPUT_IMAGE_PATH "input.bmp"

// Necessary headers
#include <GL/glut.h>
#include <cmath>
#include <algorithm>
#include <ctime>
#include "image_reader.h"

ImageReader input(INPUT_IMAGE_PATH);

// Used macros
#define POP_SIZE 30  // Population size
#define N 150        // Number of triangles per chromosome
#define V 3          // A triangle has 3 vertices
#define SCALE 512    // Input image, output image, window size are all 512x512
#define OPACITY 0.15 // Alpha channel value for triangles

// Random number generators, the second one is uniform
#define RND (2.0 * (double)rand_r(&seed) / RAND_MAX - 1.0)
#define U_RND ( (double)rand_r (&seed) / RAND_MAX)

typedef long long ll; // just an alias for long long data type
int epochs = 0; // number of generations
unsigned int seed = time(nullptr); // random numbers seed

// Chromosome representation: a set of N triangles of various positions/sizes/colors
struct Chromosome {
    double point[N][V][2]{};
    double color[N][4]{};
    ll fit_val{};

    unsigned char *window;
    Chromosome() { // ctor initializes memory for window
        window = (unsigned char *) malloc(sizeof(unsigned char) * input.width * input.height * 3);
    }

    // Draw *this chromosome to the screen
    void draw() {
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < N; i++) {
            glColor4f(color[i][0], color[i][1], color[i][2], color[i][3]); // RGBA
            glVertex2f(point[i][0][0], point[i][0][1]);
            glVertex2f(point[i][1][0], point[i][1][1]);
            glVertex2f(point[i][2][0], point[i][2][1]);
        }
        glEnd();
    }

    // Calculate fitness value (Mean-Square error) between *this chromosome and the input image
    ll fitness() {
        ll fit = 0;
        glClear(GL_COLOR_BUFFER_BIT);
        draw();
        glReadPixels(0, 0, input.width, input.height, GL_RGB, GL_UNSIGNED_BYTE, window);
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i = 0; i < input.width * input.height * 3; i += 3) {
            fit += ((window[i] - input.pixel[i]) * (window[i] - input.pixel[i])) +
                   ((window[i + 1] - input.pixel[i + 1]) * (window[i + 1] - input.pixel[i + 1])) +
                   ((window[i + 2] - input.pixel[i + 2]) * (window[i + 2] - input.pixel[i + 2]));
        }
        return fit;
    }

    // Sorting key, a chromosome is better than another if it has a lower fit value
    static bool key(const Chromosome &a, const Chromosome &b) {
        return a.fit_val < b.fit_val;
    }

    // Mutate *this chromosome by completely changing its position and color
    void mutate_change() {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < V; j++) {
                if (U_RND > 0.5f) point[i][j][0] = U_RND;
                if (U_RND > 0.5f) point[i][j][1] = U_RND;
            }
            if (U_RND > 0.5f) {
                color[i][0] = U_RND;
                color[i][1] = U_RND;
                color[i][2] = U_RND;
            }
        }
    }

    // Mutate *this chromosome by introducing small disturbance its position and color, making sure we don't go off-bounds
    void mutate_disturb(double disturb) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < V; k++) {
                if (U_RND < 0.25f) {
                    point[j][k][0] += RND / disturb;
                    point[j][k][1] += RND / disturb;
                }
                if (point[j][k][0] < .0f || point[j][k][0] > 1.f) {
                    point[j][k][0] = U_RND;
                }
                if (point[j][k][1] < .0f || point[j][k][1] > 1.f) {
                    point[j][k][1] = U_RND;
                }
            }
            if (U_RND < 0.5f) {
                color[j][0] += 10 * RND / disturb;
                color[j][1] += 10 * RND / disturb;
                color[j][2] += 10 * RND / disturb;
            }
            if (color[j][0] < .0f || color[j][0] > 1.f) color[j][0] = U_RND;
            if (color[j][1] < .0f || color[j][1] > 1.f) color[j][1] = U_RND;
            if (color[j][2] < .0f || color[j][2] > 1.f) color[j][2] = U_RND;
        }
    }
};

// One-point Crossover, chooses a random point p, swaps DNA before p and leaves the rest unmodified
void one_point_co(const Chromosome a, const Chromosome b, Chromosome &c) {
    int p = ceil(U_RND * N);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < V; j++) {
            if (i < p) {
                c.point[i][j][0] = a.point[i][j][0];
                c.point[i][j][1] = a.point[i][j][1];
            } else {
                c.point[i][j][0] = b.point[i][j][0];
                c.point[i][j][1] = b.point[i][j][1];
            }
        }
    }
    for (int i = 0; i < N; i++) {
        if (i < p) {
            c.color[i][0] = a.color[i][0];
            c.color[i][1] = a.color[i][1];
            c.color[i][2] = a.color[i][2];
        } else {
            c.color[i][0] = b.color[i][0];
            c.color[i][1] = b.color[i][1];
            c.color[i][2] = b.color[i][2];
        }
    }
}

// N-points crossover, flips a coin and swaps/leaves the DNA element (triangles)
void n_points_co(const Chromosome a, const Chromosome b, Chromosome &c) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < V; j++) {
            if (U_RND < 0.5) {
                c.point[i][j][0] = a.point[i][j][0];
                c.point[i][j][1] = a.point[i][j][1];
                c.color[i][0] = a.color[i][0];
                c.color[i][1] = a.color[i][1];
                c.color[i][2] = a.color[i][2];
            } else {
                c.point[i][j][0] = b.point[i][j][0];
                c.point[i][j][1] = b.point[i][j][1];
                c.color[i][0] = b.color[i][0];
                c.color[i][1] = b.color[i][1];
                c.color[i][2] = b.color[i][2];
            }
        }
    }
}

// Population is represented as an array of struct Chromosome.
Chromosome population[POP_SIZE];

// Generates an initial random population
void gen_pop(Chromosome *pop) {
    for (int i = 0; i < POP_SIZE; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < V; k++) {
                pop[i].point[j][k][0] = U_RND;
                pop[i].point[j][k][1] = U_RND;
            }
            pop[i].color[j][0] = U_RND;
            pop[i].color[j][1] = U_RND;
            pop[i].color[j][2] = U_RND;
            pop[i].color[j][3] = OPACITY;
        }
    }
}

// Display function used by OpenGL, called to update screen when a window even is received
// Number of calls per second defined the FPS
void gl_display() {
    glViewport(0, 0, SCALE, SCALE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    population[0].draw(); // visualize only the best chromosome
    glutSwapBuffers();
}

// OpenGL idle function, called when no window events are being received
// Implements the selection strategy of the algorithm, updating the population chromosomes
// Number of calls per second defines the generation rate, a call represents one generation.
void gl_idle() {
    epochs++;

    // Sort the population based on the fitness_value
    std::sort(population, population + POP_SIZE, Chromosome::key);

    // Best 25% of the population advances to the next generation without modification
    // Rest 75% of the population are being mutated or crossover-ed by this loop
    for (int i = POP_SIZE - ceil(POP_SIZE * 0.75); i < POP_SIZE; i++) {
        if (U_RND < 0.95) { // 95% probability to do crossover
            // select two random individuals
            int a = ((int) round(U_RND * POP_SIZE)) % POP_SIZE;
            int b = ((int) round(U_RND * POP_SIZE)) % POP_SIZE;

            // 50% probability to do one-point crossover, 50% - n-points crossover
            if (U_RND < 0.5) one_point_co(population[a], population[b], population[i]);
            else n_points_co(population[a], population[b], population[i]);

        } else {
            if (U_RND < 0.95) // 95% probability to do disturb mutation, 5% - complete change
                population[i].mutate_disturb(500 * RND);
            else population[i].mutate_change();
        }

        // Calculate new fitness value for each modified chromosome and store it as a field to be used later for sorting
        population[i].fit_val = population[i].fitness();
    }
    glutPostRedisplay();
}

// Main entry point of the program, initializes window, population, and runs the main visualization loop
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(SCALE, SCALE);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("GeneticArt");

    srand(time(nullptr));

    for (auto &i : population) i = Chromosome();
    gen_pop(population);

    for (auto &i : population) i.fit_val = i.fitness();
    std::sort(population, population + POP_SIZE, Chromosome::key);

    glutDisplayFunc(gl_display);
    glutIdleFunc(gl_idle);
    glutMainLoop();
}

