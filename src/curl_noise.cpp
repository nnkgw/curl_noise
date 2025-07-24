#if defined(WIN32)
#pragma warning(disable:4996)
#include <GL/glut.h>
#include <GL/freeglut.h>
#ifdef NDEBUG
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif // NDEBUG

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#elif defined(__APPLE__) || defined(MACOSX)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else // MACOSX
#include <GL/glut.h>
#include <GL/freeglut.h>
#endif // unix

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <iostream>

// Sphere
glm::vec3 ballPos(0.0f, 0.2f, 0.0f);
glm::vec3 ballVel(0.0f);
const float radius = 0.2f;
const float gravity = -0.5f;
const float dt = 0.016f;

// Camera
float camTheta = 0.0f;   // Horizontal rotation
float camPhi = 30.0f;    // Elevation angle
int lastX, lastY;
bool rotating = false;

glm::vec3 curlNoise(float x, float z, float t) {
  float eps = 0.1f;
  float n1 = glm::perlin(glm::vec3(x,     0.0f, z + eps + t));
  float n2 = glm::perlin(glm::vec3(x,     0.0f, z - eps + t));
  float n3 = glm::perlin(glm::vec3(x + eps, 0.0f, z + t));
  float n4 = glm::perlin(glm::vec3(x - eps, 0.0f, z + t));

  float dy = (n1 - n2) / (2.0f * eps);
  float dx = (n3 - n4) / (2.0f * eps);

  glm::vec3 v(dx, 1.0f, -dy);
  return glm::normalize(v) * 1.5f;
}

void update() {
  float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
  glm::vec3 flow = curlNoise(ballPos.x, ballPos.z, time);
  ballVel += flow * dt;
  ballVel.y += gravity * dt;
  ballPos += ballVel * dt;

  if (ballPos.y < radius) {
    ballPos.y = radius;
    if (ballVel.y < 0.0f) ballVel.y *= -0.3f;
  }

  glutPostRedisplay();
}

void drawGround() {
  glColor3f(0.1f, 0.1f, 0.1f);
  glBegin(GL_QUADS);
  glVertex3f(-2, 0, -2);
  glVertex3f( 2, 0, -2);
  glVertex3f( 2, 0,  2);
  glVertex3f(-2, 0,  2);
  glEnd();
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float r = 3.5f;
  float eyeX = r * sinf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  float eyeY = r * sinf(glm::radians(camPhi));
  float eyeZ = r * cosf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  gluLookAt(eyeX, eyeY, eyeZ, 0, 0.5, 0, 0, 1, 0);

  drawGround();

  glPushMatrix();
  glTranslatef(ballPos.x, ballPos.y, ballPos.z);
  glColor3f(0.8f, 0.3f, 0.3f);
  glutSolidSphere(radius, 32, 32);
  glPopMatrix();

  glutSwapBuffers();
}

void idle() {
  update();
}

void reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w / h, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    rotating = (state == GLUT_DOWN);
    lastX = x;
    lastY = y;
  }
}

void motion(int x, int y) {
  if (rotating) {
    camTheta += (x - lastX) * 0.5f;
    camPhi   += (y - lastY) * 0.5f;
    camPhi = glm::clamp(camPhi, 5.0f, 85.0f);
    lastX = x;
    lastY = y;
    glutPostRedisplay();
  }
}

void init() {
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.9f, 0.9f, 0.95f, 1.0f);
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Curl-Noise");

  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glutMainLoop();
  return 0;
}
