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
#include <vector>
#include <cmath>
#include <iostream>

// Fixed sphere position
glm::vec3 ballPos(0.0f, 1.0f, 0.0f);
const float radius = 0.3f;

// Camera control
float camTheta = 0.0f; // Azimuth
float camPhi = 30.0f;  // Elevation
int lastX, lastY;
bool rotating = false;

// Particle structure
struct Particle {
  glm::vec3 pos, prev;
  Particle(glm::vec3 p) : pos(p), prev(p) {}
};
std::vector<Particle> particles;

const float dt = 0.016f;
const int MAX_PARTICLES = 1000;
const float gravity = -0.2f;
const float height_limit = 3.0f;

// Initialize particle field
void initParticles() {
  particles.clear();
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    float x = ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.8f;
    float z = ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.8f;
    float y = (std::rand() % 1000) / 1000.0f * 0.5f;
    particles.emplace_back(glm::vec3(x, y, z));
  }
}

// Curl noise based velocity field
glm::vec3 curlNoise(float x, float z, float t) {
  float eps = 0.1f;
  float n1 = glm::perlin(glm::vec3(x, 0.0f, z + eps + t));
  float n2 = glm::perlin(glm::vec3(x, 0.0f, z - eps + t));
  float n3 = glm::perlin(glm::vec3(x + eps, 0.0f, z + t));
  float n4 = glm::perlin(glm::vec3(x - eps, 0.0f, z + t));

  float dy = (n1 - n2) / (2.0f * eps);
  float dx = (n3 - n4) / (2.0f * eps);

  glm::vec3 v(dx, 1.0f, -dy);
  return glm::normalize(v) * 1.5f;
}

// Update particle positions and handle sphere collision
void updateParticles() {
  float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
  for (auto& p : particles) {
    p.prev = p.pos;

    glm::vec3 flow = curlNoise(p.pos.x, p.pos.z, time);
    flow.y += gravity;

    glm::vec3 toCenter = p.pos - ballPos;
    float dist = glm::length(toCenter);
    if (dist < radius) {
      glm::vec3 n = glm::normalize(toCenter);
      glm::vec3 tangential = flow - glm::dot(flow, n) * n;
      flow = glm::normalize(tangential) * glm::length(flow);
      p.pos = ballPos + n * radius;
    }

    p.pos += flow * dt;

    if (p.pos.y > height_limit || glm::length(p.pos) > 5.0f) {
      p.pos = p.prev = glm::vec3(
        ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.8f,
        0.0f,
        ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.8f);
    }
  }
}

// Draw ground plane
void drawGround() {
  glColor3f(0.9f, 0.9f, 0.95f);
  glBegin(GL_QUADS);
  glVertex3f(-2, 0, -2);
  glVertex3f(2, 0, -2);
  glVertex3f(2, 0, 2);
  glVertex3f(-2, 0, 2);
  glEnd();
}

// Draw particles as flow lines
void drawParticles() {
  glColor3f(0.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (auto& p : particles) {
    glVertex3fv(&p.prev.x);
    glVertex3fv(&p.pos.x);
  }
  glEnd();
}

// Render callback
void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float r = 4.0f;
  float ex = r * sinf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  float ey = r * sinf(glm::radians(camPhi));
  float ez = r * cosf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  gluLookAt(ex, ey, ez, 0, 1.0, 0, 0, 1, 0);

  drawGround();

  // Draw sphere
  glPushMatrix();
  glTranslatef(ballPos.x, ballPos.y, ballPos.z);
  glColor3f(0.9f, 0.8f, 0.2f);
  glutSolidSphere(radius, 32, 32);
  glPopMatrix();

  drawParticles();
  glutSwapBuffers();
}

// Idle update
void idle() {
  updateParticles();
  glutPostRedisplay();
}

// Window reshape
void reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w / h, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

// Mouse button callback
void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    rotating = (state == GLUT_DOWN);
    lastX = x;
    lastY = y;
  }
}

// Mouse motion callback
void motion(int x, int y) {
  if (rotating) {
    camTheta += (x - lastX) * 0.5f;
    camPhi += (y - lastY) * 0.5f;
    camPhi = glm::clamp(camPhi, 5.0f, 85.0f);
    lastX = x;
    lastY = y;
    glutPostRedisplay();
  }
}

// OpenGL/GLUT init
void init() {
  glEnable(GL_DEPTH_TEST);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  initParticles();
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Curl-Noise Flow Simulation");

  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glutMainLoop();
  return 0;
}
