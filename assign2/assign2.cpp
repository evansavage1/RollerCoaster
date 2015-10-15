// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 480 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	C++ starter code
*/

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>

/*Window informations*/
const int WIDTH = 640;
const int HEIGHT = 480;
const char * WINDOW_TITLE = "RollerCoaster";

/*Camera values*/
const float FOVY = 70.0f;

/*Interaction variable definitions*/
int g_iMenuId;
int g_vMousePos[2] = { 0, 0 };
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
/* Arbitrary good initial values*/
float g_vLandRotate[3] = { 0.0, 0.0, 0.0 };
float g_vLandTranslate[3] = { 0.0, 0.0, 0.0 };
float g_vLandScale[3] = { 1.0, 1.0, 1.0 };

/* Recording control*/
bool isRecording = false;
int nPictures = 0;
char fileName[] = "CSCI420-RollerCoaster";

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

/* Spline infos*/
point * splinePoints;
int numOfSplinePoints;
const int UDIVISOR = 1000;

GLuint texture[6];
/*Skybox infos*/
char * skyBoxTopTextName = "Textures/skyTop.jpg";
char * skyBoxRightTextName = "Textures/skyRight.jpg";
char * skyBoxLeftTextName = "Textures/skyLeft.jpg";
char * skyBoxFrontTextName = "Textures/skyFront.jpg";
char * skyBoxRearTextName = "Textures/skyRear.jpg";

/*Ground infos*/
char * groundTexName = "Textures/ground.jpg";


/*Splines functions*/

/*Implements the formula to calculate the spline point
Reference: http://www.mvps.org/directx/articles/catmull/ */
double splineAxisPoint(double v1, double v2, double v3, double v4, double u){
	return (2 * v2) + (-v1 + v3) * u + (2 * v1 - 5 * v2 + 4 * v3 - v4) * pow(u, 2) + (-v1 + 3 * v2 - 3 * v3 + v4) * pow(u, 3);
}
point splineCalc(point p1, point p2, point p3, point p4, double u){
	point out;
	out.x = 0.5 * splineAxisPoint(p1.x, p2.x, p3.x, p4.x, u);
	//Inverted for good purpose. :)
	out.z = 0.5 * splineAxisPoint(p1.y, p2.y, p3.y, p4.y, u);
	out.y = 0.5 * splineAxisPoint(p1.z, p2.z, p3.z, p4.z, u);
	return out;
}

/*Calculate all spline information*/
void generateSplines(spline * splines){
	int numOfControlPoints = splines[0].numControlPoints;
	numOfSplinePoints = numOfControlPoints * UDIVISOR;
	double uInc = 1.0 / UDIVISOR;

	/*Initialize arrays*/
	splinePoints = new point[numOfSplinePoints];


	/*Brute force calculation*/
	int splineIndex = 0;
	for (int i = 0; i < numOfControlPoints - 3; i++){
		for (double u = 0.0f; u < 1.0; u += uInc){
			point p0, p1, p2, p3;
			p0 = splines[0].points[i];
			p1 = splines[0].points[i + 1];
			p2 = splines[0].points[i + 2];
			p3 = splines[0].points[i + 3];

			//calc point
			point splinePoint = splineCalc(p0, p1, p2, p3, u);

			splinePoints[splineIndex] = splinePoint;
			splineIndex++;
		}
	}

	numOfSplinePoints = splineIndex;
}

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);

	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

/*Recording stuff*/
/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	int i, j;
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc(640, 480, 3, NULL);

	printf("File to save to: %s\n", filename);

	for (i = 479; i >= 0; i--) {
		glReadPixels(0, 479 - i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
			&in->pix[i*in->nx*in->bpp]);
	}

	if (jpeg_write(filename, in))
		printf("File saved Successfully\n");
	else
		printf("Error in Saving\n");

	pic_free(in);
}

/*Drawing Functions*/
void drawGround(){
	glColor3f(1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 100.0f); glVertex3f(100.0f, -2.0f, 100.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(100.0f, -2.0f, -100.0f);
	glTexCoord2f(100.0f, 100.0f); glVertex3f(-100.0f, -2.0f, -100.0f);
	glTexCoord2f(100.0f, 0.0f); glVertex3f(-100.0f, -2.0f, 100.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void drawSkybox(){
	glColor3f(1.0, 1.0, 1.0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Top
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-100.0f, 98.0f, 100.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-100.0f, 98.0f, -100.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(100.0f, 98.0f, -100.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, 98.0f, 100.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);


	//Right

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, -2.0f, 100.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-100.0f, -2.0f, 100.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-100.0f, 98.0f, 100.0f);
	glTexCoord2f(1.0f, 0.0f);  glVertex3f(100.0f, 98.0f, 100.0f);
	
	glEnd();

	glDisable(GL_TEXTURE_2D);

	//Left
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(100.0f, -2.0f, -100.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-100.0f, -2.0f, -100.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-100.0f, 98.0f, -100.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(100.0f, 98.0f, -100.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	//Front
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[4]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(100.0f, 98.0f, 100.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(100.0f, 98.0f, -100.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, -2.0f, -100.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(100.0f, -2.0f, 100.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	//Rear
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[5]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-100.0f, 98.0f, 100.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-100.0f, 98.0f, -100.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-100.0f, -2.0f, -100.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-100.0f, -2.0f, 100.0f);
	
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void drawSpline(){

	//*Draw Lines
	glColor3f(1.0, 0.5, 0.5);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
	glLineWidth(10);
	for (int i = 0; i < numOfSplinePoints; i++){
		glVertex3f(splinePoints[i].x, splinePoints[i].y, splinePoints[i].z);
	}
	glEnd();
}

//Texture loading
void texload(int i, char * filename){
	Pic * img;
	img = jpeg_read(filename, NULL);
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
	pic_free(img);
}


/*OPENGL Callbacks*/
void display(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	/* Transform to the current world state */
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
	glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
	glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

	/*Draw calls*/
	drawSkybox();
	drawGround();
	drawSpline();

	glutSwapBuffers();
}

void doIdle()
{
	/* recording stuff */
	if (isRecording)
	{
		char myFilenm[100];
		sprintf_s(myFilenm, "anim.%04d.jpg", nPictures);
		saveScreenshot(myFilenm);
		nPictures++;
	}

	/* make the screen update */
	glutPostRedisplay();
}

void menufunc(int value)
{
	switch (value)
	{
	case 0:
		exit(0);
		break;
	}
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y)
{
	int vMouseDelta[2] = { x - g_vMousePos[0], y - g_vMousePos[1] };

	switch (g_ControlState)
	{
	case TRANSLATE:
		if (g_iLeftMouseButton)
		{
			g_vLandTranslate[0] += vMouseDelta[0] * 0.01;
			g_vLandTranslate[1] -= vMouseDelta[1] * 0.01;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandTranslate[2] += vMouseDelta[1] * 0.01;
		}
		break;
	case ROTATE:
		if (g_iLeftMouseButton)
		{
			g_vLandRotate[0] += vMouseDelta[1];
			g_vLandRotate[1] += vMouseDelta[0];
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandRotate[2] += vMouseDelta[1];
		}
		break;
	case SCALE:
		if (g_iLeftMouseButton)
		{
			g_vLandScale[0] *= 1.0 + vMouseDelta[0] * 0.01;
			g_vLandScale[1] *= 1.0 - vMouseDelta[1] * 0.01;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandScale[2] *= 1.0 - vMouseDelta[1] * 0.01;
		}
		break;
	}
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		g_iLeftMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		g_iMiddleMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		g_iRightMouseButton = (state == GLUT_DOWN);
		break;
	}

	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		g_ControlState = TRANSLATE;
		break;
	case GLUT_ACTIVE_SHIFT:
		g_ControlState = SCALE;
		break;
	default:
		g_ControlState = ROTATE;
		break;
	}

	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}


void myinit()
{
	/*Splines init*/
	generateSplines(g_Splines);

	/*Texture loading*/
	glGenTextures(6, texture);
	texload(0, groundTexName);
	texload(1, skyBoxTopTextName);
	texload(2, skyBoxRightTextName);
	texload(3, skyBoxLeftTextName);
	texload(4, skyBoxFrontTextName);
	texload(5, skyBoxRearTextName);

	/* setup gl view here */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glEnable(GL_DEPTH_TEST);

}

/* set projection to aspect ratio of window */
void reshape(int w, int h){
	GLfloat aspect = (GLfloat)w / (GLfloat)h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(FOVY, aspect, 0.01, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to track.txt.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your track file name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	//Load splines file
	loadSplines(argv[1]);

	glutInit(&argc, (char**)argv);

	/*
	create a window here..should be double buffered and use depth testing
	the code past here will segfault if you don't have a window set up....
	replace the exit once you add those calls.
	*/
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	//Window creation
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Height Field");

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for reshape*/
	glutReshapeFunc(reshape);

	/* do initialization */
	myinit();

	glutMainLoop();

	return 0;
}