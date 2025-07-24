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

// Sphere (center of vortex)
glm::vec3 ballPos(0.0f, 1.0f, 0.0f);
const float radius = 0.3f;

// Camera control
float camTheta = 0.0f;
float camPhi = 30.0f;
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

// Initialize random particle positions near the bottom
void initParticles() {
  particles.clear();
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    float x = ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.1f;
    float z = ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.1f;
    float y = (std::rand() % 1000) / 1000.0f * 0.5f;
    particles.emplace_back(glm::vec3(x, y, z));
  }
}

// Generate a vortex vector field centered around the sphere
glm::vec3 vortexFlow(glm::vec3 p) {
  glm::vec3 rel = p - ballPos;
  glm::vec2 r(rel.x, rel.z);
  float d = glm::length(r);
  if (d < 1e-3f) return glm::vec3(0, 1, 0);
  glm::vec2 swirl(-r.y, r.x); // perpendicular in XZ
  swirl = glm::normalize(swirl);
  glm::vec3 v(swirl.x, 1.0f, swirl.y);
  return glm::normalize(v) * 1.2f;
}

// (Optional) Add curl noise to enhance flow naturalness
glm::vec3 curlNoise(float x, float z, float t) {
  float eps = 0.1f;
  float n1 = glm::perlin(glm::vec3(x, 0.0f, z + eps + t));
  float n2 = glm::perlin(glm::vec3(x, 0.0f, z - eps + t));
  float n3 = glm::perlin(glm::vec3(x + eps, 0.0f, z + t));
  float n4 = glm::perlin(glm::vec3(x - eps, 0.0f, z + t));

  float dy = (n1 - n2) / (2.0f * eps);
  float dx = (n3 - n4) / (2.0f * eps);

  glm::vec3 v(dx, 1.0f, -dy);
  return glm::normalize(v) * 0.3f; // reduced strength
}


// Vector potential field psi(p): Eq.1 in Bridson SIGGRAPH 2007
glm::vec3 potential(glm::vec3 p) {
  float scale = 0.5f;
  float psi1 = glm::perlin(p * scale + glm::vec3(31.416f, 0.0f, 47.853f));
  float psi2 = glm::perlin(p * scale + glm::vec3(0.0f, 19.943f, 12.345f));
  float psi3 = glm::perlin(p * scale + glm::vec3(3.142f, 1.618f, 2.718f));
  return glm::vec3(psi1, psi2, psi3);
}

// Curl of vector potential psi(p) using central differences
glm::vec3 curl(glm::vec3 p) {
  float eps = 0.01f;

  float dpsi3_dy = (potential(p + glm::vec3(0, eps, 0)).z - potential(p - glm::vec3(0, eps, 0)).z) / (2.0f * eps);
  float dpsi2_dz = (potential(p + glm::vec3(0, 0, eps)).y - potential(p - glm::vec3(0, 0, eps)).y) / (2.0f * eps);
  float dpsi1_dz = (potential(p + glm::vec3(0, 0, eps)).x - potential(p - glm::vec3(0, 0, eps)).x) / (2.0f * eps);
  float dpsi3_dx = (potential(p + glm::vec3(eps, 0, 0)).z - potential(p - glm::vec3(eps, 0, 0)).z) / (2.0f * eps);
  float dpsi2_dx = (potential(p + glm::vec3(eps, 0, 0)).y - potential(p - glm::vec3(eps, 0, 0)).y) / (2.0f * eps);
  float dpsi1_dy = (potential(p + glm::vec3(0, eps, 0)).x - potential(p - glm::vec3(0, eps, 0)).x) / (2.0f * eps);

  return glm::vec3(
    dpsi3_dy - dpsi2_dz,
    dpsi1_dz - dpsi3_dx,
    dpsi2_dx - dpsi1_dy
  );
}


// Particle update: vortex flow + optional noise + sliding on sphere
void updateParticles() {
  float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
  for (auto& p : particles) {
    p.prev = p.pos;

    glm::vec3 flow = curl(p.pos);
    flow += curlNoise(p.pos.x, p.pos.z, time);
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
        ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.1f,
        0.0f,
        ((std::rand() % 2000) / 1000.0f - 1.0f) * 0.1f);
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

// Draw flow lines as short trails
void drawParticles() {
  glColor3f(0.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (auto& p : particles) {
    glVertex3fv(&p.prev.x);
    glVertex3fv(&p.pos.x);
  }
  glEnd();
}

// Main display callback
void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float r = 4.0f;
  float ex = r * sinf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  float ey = r * sinf(glm::radians(camPhi));
  float ez = r * cosf(glm::radians(camTheta)) * cosf(glm::radians(camPhi));
  gluLookAt(ex, ey, ez, 0, 1.0, 0, 0, 1, 0);

  drawGround();

  glPushMatrix();
  glTranslatef(ballPos.x, ballPos.y, ballPos.z);
  glColor3f(0.9f, 0.8f, 0.2f);
  glutSolidSphere(radius, 32, 32);
  glPopMatrix();

  drawParticles();
  glutSwapBuffers();
}

// Idle callback
void idle() {
  updateParticles();
  glutPostRedisplay();
}

// Window resize
void reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w / h, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

// Mouse button
void mouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    rotating = (state == GLUT_DOWN);
    lastX = x;
    lastY = y;
  }
}

// Mouse drag
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

// Entry point
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
