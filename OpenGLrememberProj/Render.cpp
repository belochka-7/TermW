#include "Render.h"

#include <windows.h>
#include <sstream>
#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;
bool flight = false;

//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()

Shader s[10];  //массивчик для десяти шейдеров


//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	float camDist;
	//углы поворота камеры
	float fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры

//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света

//старые координаты мыши
int mouseX = 0, mouseY = 0;
float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}
	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{
	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL* ogl, int key)
{
	switch (key)
	{
	case 'L':
		lightMode = !lightMode;
		break;
	case 'T':
		textureMode = !textureMode;
		break;
	case 'R':
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
		break;
	case 'F':
		light.pos = camera.pos;
		break;
	case 'S':
		s[0].LoadShaderFromFile();
		s[0].Compile();
		break;
	case 'Q':
		Time = 0;
		break;
	case 'P':
		flight = !flight;
		tick_n = GetTickCount();
		tick_o = tick_n;
		break;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{

}


ObjFile plane;
Texture planeTex, prismTex;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);

	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	loadModel("models\\plane.obj", &plane);
	planeTex.loadTextureFromFile("textures//airplane_baseColor.bmp");
	prismTex.loadTextureFromFile("textures//0001.bmp");

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("T - вкл/выкл текстур\nL - вкл/выкл освещение\nF - Свет из камеры\nG - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали\nP - запустить самолет",0,0,0);
}

void makeVec(const float p1[3], const float p2[3], const float p3[3], float res[3]) {
	float vec1[3] = { p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2] };
	float vec2[3] = { p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2] };
	res[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
	res[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
	res[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

void setMaterial(GLfloat amb[4], GLfloat dif[4], GLfloat spec[4], GLfloat sh) {
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, sh);
	glMaterialfv(GL_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_BACK, GL_DIFFUSE, dif);
	glMaterialfv(GL_BACK, GL_SPECULAR, spec);
	glMaterialf(GL_BACK, GL_SHININESS, sh);
}

float f(float a, float b, float t)
{
	return a * (1 - t) + b * t;
}

void makeConvex(float P[][3], int N, int place, const float C[3], int add)
{
	for (int i = N - 1 + add; i >= place + add; --i)
	{
		P[i][0] = P[i - add][0];
		P[i][1] = P[i - add][1];
		P[i][2] = P[i - add][2];
	}

	for (int i = place + 1; i <= place + add; ++i)
	{
		float A[3], B[3];
		float t = (i - place) * (1.0001 / (add + 2));
		A[0] = f(P[place][0], C[0], t);
		A[1] = f(P[place][1], C[1], t);
		A[2] = f(P[place][2], C[2], t);

		B[0] = f(C[0], P[place + add + 1][0], t);
		B[1] = f(C[1], P[place + add + 1][1], t);
		B[2] = f(C[2], P[place + add + 1][2], t);

		P[i][0] = f(A[0], B[0], t);
		P[i][1] = f(A[1], B[1], t);
		P[i][2] = f(A[2], B[2], t);
	}
}

Vector3 bezeirPoint(float P[][3], float t)
{
	float t2 = t * t, t3 = t2 * t;
	float tI = 1 - t, tI2 = tI * tI, tI3 = tI2 * tI;
	return Vector3(tI3 * P[0][0] + 3 * t * tI2 * P[1][0] + 3 * t2 * tI * P[2][0] + t3 * P[3][0],
				   tI3 * P[0][1] + 3 * t * tI2 * P[1][1] + 3 * t2 * tI * P[2][1] + t3 * P[3][1],
				   tI3 * P[0][2] + 3 * t * tI2 * P[1][2] + 3 * t2 * tI * P[2][2] + t3 * P[3][2]);
}

Vector3 bezeirDirection(float P[][3], float t)
{
	Vector3 res = bezeirPoint(P, t+0.001);

	Vector3 tmp = bezeirPoint(P, t);
	res = res - tmp;
	return res.normalize();
}

float texA(float P1[3], float P2[3], float texX)
{
	texX = texX+hypot(P1[0]-P2[0], P1[1]-P2[1])/20, 1.0;
	return texX;
}

void drawPrism() {   
	const float P0[8][3] = {  { 0,  0, 0},
							  { 0,  7, 0},
							  { 6,  8, 0},
							  { 3,  1, 0},
							  { 7, -3, 0},
							  { 1, -1, 0},
							  {-3, -8, 0},
							  {-4, -1, 0} };
	const float C1[3] = { -5, -5, 0 };
	const float C2[3] = { 3, 6, 0 };
	const float h = 5;

	// 7x7
	int N = 28;
	float P1[28][3];
	for (int i = 0; i < 8; ++i)
	{
		P1[i][0] = P0[i][0];
		P1[i][1] = P0[i][1];
		P1[i][2] = P0[i][2];
	}

	makeConvex(P1, 8, 6, C1, 10);
	makeConvex(P1, 18, 1, C2, 10);


	float texX[28];
	texX[0] = 0;
	for (int i = 1; i < 28; ++i)
	{
		texX[i] = texA(P1[i-1], P1[i], texX[i-1]);
	}

	float P2[28][3]{}, vec[3]{};

	for (int i = 0; i < N; ++i) {
		P2[i][0] = P1[i][0];
		P2[i][1] = P1[i][1];
		P2[i][2] = P1[i][2] + h;
	}

	prismTex.bindTexture();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_NORMALIZE);
	glBegin(GL_TRIANGLES);

	GLfloat sh = 0.1f * 256;
	GLfloat amb[] = { 0.2, 0.2, 0.5, 1. };
	GLfloat dif[] = { 0.4, 0.4, 0.4, 1. };
	GLfloat spec[] = { 0.9, 0.9, 0.9, 1. };
	setMaterial(amb, dif, spec, sh);

	makeVec(P1[0], P1[1], P2[0], vec);
	glNormal3fv(vec);
	glTexCoord2d(texX[0], 0);
	glVertex3fv(P1[0]);
	glTexCoord2d(texX[0], 0.5);
	glVertex3fv(P2[0]);
	glTexCoord2d(texX[1], 0);
	glVertex3fv(P1[1]);

	makeVec(P2[0], P1[1], P2[1], vec);
	glNormal3fv(vec);
	glTexCoord2d(texX[0], 0.5);
	glVertex3fv(P2[0]);
	glTexCoord2d(texX[1], 0.5);
	glVertex3fv(P2[1]);
	glTexCoord2d(texX[1], 0);
	glVertex3fv(P1[1]);

	for (int i = 1; i < N; ++i)
	{
		int nxt = (i + 1) % N;

		makeVec(P1[nxt], P1[i], P1[0], vec);
		glNormal3fv(vec);
		glTexCoord2d((P1[0][0] + 4) / 15, (P1[0][1] + 8) / 15);
		glVertex3fv(P1[0]);
		glTexCoord2d((P1[i][0] + 4) / 15, (P1[i][1] + 8) / 15);
		glVertex3fv(P1[i]);
		glTexCoord2d((P1[nxt][0] + 4) / 15, (P1[nxt][1] + 8) / 15);
		glVertex3fv(P1[nxt]);

		makeVec(P1[i], P1[nxt], P2[i], vec);
		glNormal3fv(vec);
		glTexCoord2d(texX[i], 0);
		glVertex3fv(P1[i]);
		glTexCoord2d(texX[i], 0.5);
		glVertex3fv(P2[i]);
		glTexCoord2d(texX[nxt], 0);
		glVertex3fv(P1[nxt]);

		makeVec(P2[i], P1[nxt], P2[nxt], vec);
		glNormal3fv(vec);
		glTexCoord2d(texX[i], 0.5);
		glVertex3fv(P2[i]);
		glTexCoord2d(texX[nxt], 0.5);
		glVertex3fv(P2[nxt]);
		glTexCoord2d(texX[nxt], 0);
		glVertex3fv(P1[nxt]);
	}

	GLfloat ambA[] = { 0.2, 0.2, 0.1, 0.75 };
	GLfloat difA[] = { 0.65, 0.4, 0.5, 0.75 };
	GLfloat specA[] = { 0.9, 0.8, 0.3, 0.75 };
	setMaterial(ambA, difA, specA, sh);

	for (int i = 1; i < N; ++i)
	{
		int prv = i - 1, nxt = (i + 1) % N;

		makeVec(P2[0], P2[i], P2[nxt], vec);
		glNormal3fv(vec);
		glVertex3fv(P2[0]);
		glVertex3fv(P2[nxt]);
		glVertex3fv(P2[i]);
	}
	glEnd();
	glDisable(GL_NORMALIZE);
}

void Render(OpenGL* ogl)
{
	if (flight)
	{
		tick_o = tick_n;
		tick_n = GetTickCount();
		Time += (tick_n - tick_o) / 1000.0;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);

	//альфаналожение
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//===================================
	//Прогать тут  
	GLfloat sh = 0.1f * 256;
	GLfloat amb[] = { 0.2, 0.2, 0.2, 1. };
	GLfloat dif[] = { 0.7, 0.7, 0.7, 1. };
	GLfloat spec[] = { 0.9, 0.9, 0.9, 1. };
	setMaterial(amb, dif, spec, sh);

	Vector3 pos;
	float sequenceL[5] = { 5.0, 4.0, 3.0, 2.0, 1.0 };
	float sequenceP[5][4][3]{ { {-1, -2,  5}, {  2, -1,  5}, {  3,  6,  5}, {-3,  9,  7} },
							  { {-3,  9,  7}, { -6, 10,  8}, { -8,  8,  9}, {-9,  6,  9} },
							  { {-9,  6,  9}, {-10,  5,  9}, {-10,  0,  9}, {-7, -3,  8} },
							  { {-7, -3,  8}, { -6, -4,  7}, { -5, -4,  7}, {-3, -3,  6} },
							  { {-3, -3,  6}, { -2, -3,  5}, { -1, -2,  5}, {-1, -2,  5} } };
	int sequenceI = 0;
	float usedTime;
	for (usedTime = 0; usedTime+sequenceL[sequenceI] < Time && sequenceI < 5; usedTime += sequenceL[sequenceI-1])
		++sequenceI;
	if (sequenceI >= 5)
	{
		Time = 0;
		flight = false;
	}

	pos = bezeirPoint(sequenceP[sequenceI], (Time-usedTime)/sequenceL[sequenceI]);

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 5; ++i)
	{
		for (float t = 0; t < 1.001; t += 0.1)
		{
			Vector3 point = bezeirPoint(sequenceP[i], t);
			glVertex3dv(point.toArray());
		}
	}
	glEnd();

	glActiveTexture(GL_TEXTURE0);
	planeTex.bindTexture();
	PUSH;

	Vector3 direction = bezeirDirection(sequenceP[sequenceI], (Time - usedTime) / sequenceL[sequenceI]);
	Vector3 x(1, 0, 0);
	Vector3 axis = x.cross(direction);

	glTranslated(pos.X(), pos.Y(), pos.Z());
	glRotated(acos(x.dot(direction)) * 180 / M_PI, axis.X(), axis.Y(), axis.Z());
	glScaled(0.05, 0.05, 0.05);
	plane.RenderModel(GL_TRIANGLES);

	POP;

	drawPrism();
}

bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();

	Shader::DontUseShaders(); 
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

