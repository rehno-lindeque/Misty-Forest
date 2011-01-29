#include <GL/gl.h>
#ifdef _WIN32
  #include "GL/glut.h"
#else
  #include <GL/glut.h>
#endif

#include <stdlib.h>
#include <iostream>
#include <list>

#include <common/types.h>
#include <common/math/math.h>

#include "tga.h"


#define PI 3.14159265358979323846

#define ESCAPE 27				// Define ASCII value for special


bool keys[256];

GLint windowWidth = 640, windowHeight = 480; // Window dimensions


const int RANGE_MULTIPLIER = 1;
const float WORLD_RANGE = 20.0 * RANGE_MULTIPLIER;
const float WORLD_HEIGHT = 20.0;// * RANGE_MULTIPLIER;
const int N_TREES = 200 * RANGE_MULTIPLIER * RANGE_MULTIPLIER;
const int N_FIREFLIES = 100 * RANGE_MULTIPLIER * RANGE_MULTIPLIER;// * RANGE_MULTIPLIER;

class Textures
{
protected:
  static const int N_TEXTURES;
  static const char* FILENAMES[/*Textures::N_TEXTURES*/5];
  GLuint textures[/*Textures::N_TEXTURES*/5];

public:
  bool loadTexture(const char* filename, int textureIndex)
  {
    TGAImg tgaImage;
    if(tgaImage.Load(filename) != IMG_OK)
      return false;
    glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);
    if(tgaImage.GetBPP() == 24)
      glTexImage2D(GL_TEXTURE_2D, 0, 3, tgaImage.GetWidth(), tgaImage.GetHeight(), 0, GL_RGB,GL_UNSIGNED_BYTE, tgaImage.GetImg());
    else if(tgaImage.GetBPP() == 32)
      glTexImage2D(GL_TEXTURE_2D, 0, 4, tgaImage.GetWidth(), tgaImage.GetHeight(), 0, GL_RGBA,GL_UNSIGNED_BYTE, tgaImage.GetImg());    else      return false;

    // Specify filtering and edge actions
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return true;
  }

  void load()
  {
    glGenTextures(N_TEXTURES, textures);

    for(int c = 0; c < N_TEXTURES; c++)
      loadTexture(FILENAMES[c], c);
  }

  inline GLuint get(int index) const { return textures[index]; }

} textures;
const int Textures::N_TEXTURES = 5;
const char* Textures::FILENAMES[Textures::N_TEXTURES] = { "data/forrestground.tga", "data/firefly.tga", "data/sky.tga", "data/wood.tga", "data/leaves.tga" };


class Avatar
{
public:
	Vector3 position;
	float angle;

	Avatar() : position(0.0f, 1.5f, 0.0f), angle((float)PI/4)
	{}

	void draw()
	{
		glPushMatrix();
		//glLoadIdentity();
		glTranslatef(position(0), position(1), position(2));
		glRotatef(angle/PI*180.0f, 0.0f, 1.0f, 0.0f);
		glutWireSphere(0.5f, 10, 10);
		glPopMatrix();
	}
} avatar;

class Camera
{
public:
	enum Type
	{ THIRD_PERSON, FIRST_PERSON } type;
	Vector3 position;
	Vector3 lookAt;
	int width, height;
  float followDistance;

	Camera()
	{
		type = THIRD_PERSON;

		position(0) = 0.0;
		position(1) = 2.0;
		position(2) = -4.0;

		lookAt(0) = 0.0;
		//lookAt(1) = -0.5;
		lookAt(1) = 0.0;
		//lookAt(2) = 1.0;
		lookAt(2) = 0.0;
		//lookAt.normalize();

    followDistance = sqrt(33.0);
    //followDistance = 100.0f;
	}

	void update()
	{
    glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
    gluPerspective(45.0, (GLfloat)width/height, 0.01, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
		switch(type)
		{
		case THIRD_PERSON:
		  {
			  lookAt = avatar.position;

			  Vector3 relativePos(-sin(avatar.angle), 0.8, -cos(avatar.angle));
			  relativePos.normalize();
			  relativePos *= followDistance;
			  position = avatar.position + relativePos;
        gluLookAt(position(0), position(1), position(2), lookAt(0), lookAt(1), lookAt(2), 0.0, 1.0, 0.0);

			  break;
		  }
		case FIRST_PERSON:
      {
        position = avatar.position;
        glRotatef(-avatar.angle/PI*180.0+180.0, 0.0, 1.0, 0.0);
        glTranslatef(-position(0), -position(1), -position(2));

			  break;
      }
		}

		glMatrixMode(GL_MODELVIEW);
	}

	void setBounds(int width, int height)
	{
		Camera::width = width;
		Camera::height = height;
		update();
	}
} camera;

class EnvironmentCylinder // this would usually be an environment sphere or box
{
public:
  void draw()
  {
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();
    glRotatef(90.0f, -1.0f, 0.0f, 0.0f);
    GLUquadricObj* cylinder = gluNewQuadric();
    gluQuadricDrawStyle(cylinder, GLU_FILL);
    gluQuadricTexture(cylinder, GLU_TRUE);
    gluQuadricOrientation(cylinder, GLU_INSIDE);
    //old: gluQuadricNormals(cylinder, GLU_SMOOTH);
    glBindTexture(GL_TEXTURE_2D, textures.get(2));
    gluCylinder(cylinder, WORLD_RANGE, WORLD_RANGE, WORLD_HEIGHT, 30, 1);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
  }

} environment;

class Ground
{
protected:
  unsigned int divisions, textureDivisions;
  float* heightMap;
  GLuint displayList;
  float radius;

public:
  /*Ground(unsigned int divisions) : divisions(divisions)
  {
    heightMap = new float[(divisions+2)*(divisions+2)];
    for(unsigned int cy = 0; cy < divisions+2; cy++)
      for(unsigned int cx = 0; cx < divisions+2; cx++)
      {
        heightMap[cy*divisions+cx] = rand()/(double)RAND_MAX;
      }
  }*/
  void init(unsigned int divisions, unsigned int textureDivisions, float radius, float height)
  {
    Ground::radius = radius;
    Ground::divisions = divisions;
    Ground::textureDivisions = textureDivisions;
    heightMap = new float[(divisions+2)*(divisions+2)];
    for(unsigned int cy = 0; cy < divisions+2; cy++)
      for(unsigned int cx = 0; cx < divisions+2; cx++)
        heightMap[cy*divisions+cx] = rand()/(double)RAND_MAX*height;

    //set up diplay list
    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textures.get(0));
      glBegin(GL_QUADS);
      glColor3f(1.0, 1.0, 1.0);
      for(uint cx = 1; cx < divisions+1; cx++)
      {
        for(uint cz = 1; cz < divisions+1; cz++)
        {
		      // topleft
          glTexCoord2fv(getUVCoordinate(cx, cz));
          glNormal3fv(getNormal(cx, cz));
          glVertex3fv(getVertex(cx, cz));
		      // bottomleft
          glTexCoord2fv(getUVCoordinate(cx, cz+1));
          glNormal3fv(getNormal(cx, cz+1));
		      glVertex3fv(getVertex(cx, cz+1));
		      // bottomright
          glTexCoord2fv(getUVCoordinate(cx+1, cz+1));
          glNormal3fv(getNormal(cx+1, cz+1));
		      glVertex3fv(getVertex(cx+1, cz+1));
		      // topright
          glTexCoord2fv(getUVCoordinate(cx+1, cz));
          glNormal3fv(getNormal(cx+1, cz));
		      glVertex3fv(getVertex(cx+1, cz));
        }
      }
	    glEnd();
    glEndList();
  }

  inline Vector2 getUVCoordinate(int xIndex, int yIndex)
  {
    return Vector2(xIndex/(float)divisions * textureDivisions, yIndex/(float)divisions * textureDivisions);
  }

  inline Vector3 getVertex(int xIndex, int yIndex)
  {
    //return Vector3(-radius/divisions+(xIndex-divisions/2.0)*2.0*radius/divisions, heightMap[yIndex*divisions+xIndex], -radius/divisions + (yIndex-divisions/2.0)*2.0*radius/divisions);
    return Vector3(-radius/divisions+(xIndex-0.5-divisions/2.0)*2.0*radius/divisions, heightMap[yIndex*divisions+xIndex], -radius/divisions + (yIndex-0.5-divisions/2.0)*2.0*radius/divisions);
  }

  inline Vector3 getFaceNormal(int xFaceIndex, int yFaceIndex)
  {
    Vector3 first = getVertex(xFaceIndex, yFaceIndex) - getVertex(xFaceIndex+1, yFaceIndex);
    Vector3 second = getVertex(xFaceIndex, yFaceIndex) - getVertex(xFaceIndex, yFaceIndex+1);
    return Vector3::cross(second, first);
  }

  inline Vector3 getNormal(int xIndex, int yIndex)
  {
    Vector3 compositeNormal = getFaceNormal(xIndex-1, yIndex-1) + getFaceNormal(xIndex, yIndex-1) + getFaceNormal(xIndex, yIndex) + getFaceNormal(xIndex-1, yIndex);
    return compositeNormal.normalize();
  }

  void draw()
  {
    //const int divisions=20;
	  glCallList(displayList);
  }

} ground;

class Tree
{
protected:
  Vector3 position;
  float angle, height, radius;
public:
  void draw()
  {
    glPushMatrix();
    //glLoadIdentity();

    //glRotatef(angle, 0.0f, 1.0f, 0.0f);
    glTranslatef(position(0), position(1), position(2));
    glRotatef(90.0f, -1.0f, 0.0f, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    GLUquadricObj* cylinder = gluNewQuadric();
    //gluQuadricDrawStyle(cylinder, GLU_LINE);
    gluQuadricDrawStyle(cylinder, GLU_FILL);
    gluQuadricTexture(cylinder, GLU_TRUE);

    //trunk
    glBindTexture(GL_TEXTURE_2D, textures.get(3));
    gluCylinder(cylinder, 0.2*radius, 0.0, 6.3*height, 6, 3);

    glDisable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.45f);
    glBindTexture(GL_TEXTURE_2D, textures.get(4));

    //lower leaves
    glTranslatef(0.0f, 0.0f, 2.0f*height);
    gluCylinder(cylinder, 0.8*radius, 0.0, 1.2*height, 7, 3);
    //middle leaves
    glTranslatef(0.0f, 0.0f, 0.8f*height);
    gluCylinder(cylinder, 0.6*radius, 0.0, 2.0*height, 7, 3);
    //top leaves
    glTranslatef(0.0f, 0.0f, 0.7f*height);
    gluCylinder(cylinder, 0.4*radius, 0.0, 3.0*height, 7, 3);
    glPopMatrix();

    glDisable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
  }
  void set(const Vector3& position, float angle, float height, float radius) { Tree::position = position; Tree::angle = angle; Tree::height = height; Tree::radius = radius; }
};

class Trees
{
protected:
  Tree trees[N_TREES];
  GLuint displayList;
public:
  void init()
  {
    for(int c = 0; c < N_TREES; c++)
      trees[c].set(Vector3((rand()/(float)RAND_MAX-0.5f)*2.0f*WORLD_RANGE, 0.0f, (rand()/(float)RAND_MAX-0.5f)*2.0f*WORLD_RANGE),
                   (rand()/(float)RAND_MAX)*360,
                   1.0f + (rand()/(float)RAND_MAX-0.5f)*2.0f*0.5f,
                   1.0f + (rand()/(float)RAND_MAX-0.5f)*2.0f*0.5f);

    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
    glColor3f(1.0f, 1.0f, 1.0f);
      for(int c = 0; c < N_TREES; c++)
        trees[c].draw();
    glEndList();
  }
  void draw()
  {
    /*for(int c = 0; c < 20; c++)
      trees[c].draw();*/
    glCallList(displayList);
  }
} trees;

class Firefly
{
public:
  static const float MAX_SPEED;
  static const float MAX_ACCELERATION;
  static const float RADIUS;
  Vector3 position;
  Vector3 velocity;
public:
  void move()
  {
    const float BOUND_DIFFERENCE = MAX_SPEED * MAX_ACCELERATION;
    Vector3 acceleration((rand()/(float)RAND_MAX-0.5)*2.0*MAX_ACCELERATION, (rand()/(float)RAND_MAX-0.5)*2.0*MAX_ACCELERATION, (rand()/(float)RAND_MAX-0.5)*2.0*MAX_ACCELERATION);

    if(position(1) > WORLD_HEIGHT - BOUND_DIFFERENCE)
      acceleration += Vector3(0.0f, -MAX_ACCELERATION, 0.0f);
    else if(position(1) < 1.0f + BOUND_DIFFERENCE)
      acceleration += Vector3(0.0f, MAX_ACCELERATION, 0.0f);

    if(position(0) < -WORLD_RANGE + BOUND_DIFFERENCE)
      acceleration += Vector3(MAX_ACCELERATION, 0.0f, 0.0f);
    else if(position(0) > WORLD_RANGE - BOUND_DIFFERENCE)
      acceleration += Vector3(-MAX_ACCELERATION, 0.0f, 0.0f);

    if(position(2) < -WORLD_RANGE + BOUND_DIFFERENCE)
      acceleration += Vector3(0.0f, 0.0f, MAX_ACCELERATION);
    else if(position(2) > WORLD_RANGE - BOUND_DIFFERENCE)
      acceleration += Vector3(0.0f, 0.0f, -MAX_ACCELERATION);

    velocity += acceleration;
    float len = velocity.getLength();
    if(len > MAX_SPEED)
      velocity *= MAX_SPEED / len;
    position += velocity;
  }

  void draw()
  {
    //Note: Could use the GL_NV_point_sprite extension

    glPushMatrix();
    glTranslatef(position(0), position(1), position(2));
    //gluLookAt(position(x), position(y), position(z), camera.position(x), camera.position(y), camera.position(z), 0.0, 1.0, 0.0);

    // calculate rotation so the billboard faces the camera
    Vector3 billboardToCameraDirection = (camera.position - position).getNormalized();
    Vector3 billboardNormal(0.0f, 0.0f, 1.0f);
    float angle = acos(Vector3::dot(billboardToCameraDirection, billboardNormal));
    Vector3 rotationAxis = Vector3::cross(billboardNormal, billboardToCameraDirection);
    glRotatef(angle/PI*180.0f, rotationAxis(0), rotationAxis(1), rotationAxis(2));

    glBegin(GL_QUADS);
      //bottomleft
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-RADIUS, -RADIUS, 0.0f);
      //bottomright
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f( RADIUS, -RADIUS, 0.0f);
      //topright
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f( RADIUS,  RADIUS, 0.0f);
      //topleft
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(-RADIUS,  RADIUS, 0.0f);
    glEnd();
    glPopMatrix();
  }

  void set(const Vector3& position, const Vector3& velocity)
  {
    Firefly::position = position;
    Firefly::velocity = velocity;
  }
};
const float Firefly::MAX_SPEED = 0.02f;
const float Firefly::MAX_ACCELERATION = 0.001f;
const float Firefly::RADIUS = 0.3f;

struct comp_firefly_distance : public binary_function <Firefly, Firefly, bool>
{
  bool operator()(const Firefly& _Left, const Firefly& _Right) const
  {
    //Vector3 billboardToCameraDirection = camera.position - position;
    float leftDistance = (camera.position - _Left.position).getLength();
    float rightDistance = (camera.position - _Right.position).getLength();
    return leftDistance > rightDistance;
  }
};

class Fireflies
{
protected:
  //Firefly fireflies[N_FIREFLIES];
  std::list<Firefly> fireflies;

public:
  Fireflies()
  {
    for(int c = 0; c < N_FIREFLIES; c++)
    {
      Firefly firefly;
      firefly.set(Vector3((rand()/(float)RAND_MAX-0.5f)*2.0f*WORLD_RANGE, (rand()/(float)RAND_MAX)*WORLD_HEIGHT, (rand()/(float)RAND_MAX-0.5f)*2.0f*WORLD_RANGE),
                  Vector3((rand()/(float)RAND_MAX-0.5f)*2.0f*Firefly::MAX_SPEED, (rand()/(float)RAND_MAX-0.5f)*2.0f*Firefly::MAX_SPEED, (rand()/(float)RAND_MAX-0.5f)*2.0f*Firefly::MAX_SPEED));
      fireflies.push_back(firefly);
    }
  }

  void draw()
  {
    glBindTexture(GL_TEXTURE_2D, textures.get(1));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    fireflies.sort(comp_firefly_distance());

    for(std::list<Firefly>::iterator i = fireflies.begin(); i != fireflies.end(); i++)
      (*i).draw();
    glDisable(GL_BLEND);
  }
  void update()
  {
    for(std::list<Firefly>::iterator i = fireflies.begin(); i != fireflies.end(); i++)
      (*i).move();
  }
}fireflies;


// Renders output to screen
void display(void)
{
	camera.update();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glColor3f(1.0,1.0,1.0);		// White colour

  glPushMatrix();	// Saves current state
	glNormal3f(0.0, 1.0, 0.0);
	//glLoadIdentity();

	// Positions the default camera so that the origin (0,0,0) is easily visible
	/*glTranslatef(0.0, 0.0, -3.5);
	glRotatef((float)xAngle, 1.0, 0.0, 0.0);
	glRotatef((float)yAngle, 0.0, 1.0, 0.0);
	glRotatef((float)zAngle, 0.0, 0.0, 1.0);*/

  environment.draw();
  //glLoadIdentity();
  ground.draw();
  trees.draw();
  glColor3f(1.0,1.0,1.0);
	//avatar.draw();
  fireflies.draw();

	glPopMatrix(); // Restores previously saved state

	glFlush();
	glutSwapBuffers();

}


// Handles all normal keyboard input
void keyInput(unsigned char key, int x, int y) {

	if (key == ESCAPE)
		exit(0);

  keys[key] = true;

	// Insert the rest of your keyboard handling code here
	switch(key)
	{
	case 'w':
		avatar.position(0) += sin(avatar.angle);
		avatar.position(2) += cos(avatar.angle);
		//camera.position(0) += cos(avatar.angle);
		//camera.position(2) += sin(avatar.angle);
		break;
	case 's':
		avatar.position(0) -= sin(avatar.angle);
		avatar.position(2) -= cos(avatar.angle);
		break;
	case 'a': avatar.angle += 0.1f; break;
	case 'd': avatar.angle -= 0.1f; break;
  case ' ':
    if(camera.type == Camera::THIRD_PERSON)
      camera.type = Camera::FIRST_PERSON;
    else
      camera.type = Camera::THIRD_PERSON;
    break;
  case 'f': camera.followDistance += 0.5; break;
  case 'r': if(camera.followDistance > 0.5f) camera.followDistance -= 0.5; break;
	}

	glutPostRedisplay();	// Sends signal to redraw the viewport
							// This should be the last call in this
							// function.
}

// Handles basic initialisation and setup of OpenGL parameters.
void myinit()
{
	srand(4);
  //misc
	glClearColor(0.5f, 0.6f, 0.7f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(2.0);
	glEnable(GL_CULL_FACE);

  // lighting
  //glShadeModel(GL_FLAT);
  glShadeModel(GL_SMOOTH);
	//glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_LIGHTING);
	{
		glEnable(GL_LIGHT0);
		float pos[4] = { 20.0, 10.0, 20.0, 1.0 };
		float dir[4] = { 0.0, -1.0, 0.0, 0.0 };
		float ambient[4] = {0.0, 0.0, 0.0, 1.0};
		float diffuse[4] = {0.8, 0.8, 0.8, 1.0};
		//float specular[4] = {1.0, 1.0, 1.0, 1.0};

		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		//glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	}

	{
		glEnable(GL_LIGHT1);
		float pos[4] = { 20.0, 20.0, -20.0, 1.0 };
		float dir[4] = { 0.0, -1.0, 0.0, 0.0 };
		float ambient[4] = {0.0, 0.0, 0.0, 1.0};
		float diffuse[4] = {0.4, 0.6, 0.8, 1.0};
		//float specular[4] = {1.0, 1.0, 1.0, 1.0};

		glLightfv(GL_LIGHT1, GL_POSITION, pos);
		glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
		//glLightfv(GL_LIGHT1, GL_SPECULAR, specular);
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

  // fog
  GLfloat fogColor[4] = {0.5f, 0.6f, 0.7f, 0.0f};
  glEnable(GL_FOG);
  glFogf(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 0.05f);
  glFogfv(GL_FOG_COLOR, fogColor);
  /*glFogf(GL_FOG_START, 1.0f);
  glFogf(GL_FOG_END, 5.0f);*/
  glHint(GL_FOG_HINT, GL_DONT_CARE);

  // texturing
  glEnable(GL_TEXTURE_2D);
  textures.load();

  // set up display lists
  trees.init();
  ground.init(20, 5, WORLD_RANGE, 1.0f);
}

// Rendered window's reshape function - manages changes to size of the
// window, correctly adjusting aspect ratio.
void changeParentWindow(GLsizei width, GLsizei height)
{
	if(height == 0)
    height = 1;
	glViewport(0, 0, width, height);
	camera.setBounds(width, height);

  glLoadIdentity();
}

void idle(void)
{
  fireflies.update();
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
  memset(keys, 0, sizeof(keys));
  glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("23123576 Rehno Lindeque");

  glutReshapeFunc(changeParentWindow);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyInput);
	glutIdleFunc(idle);

	myinit();
  glutMainLoop();
}
