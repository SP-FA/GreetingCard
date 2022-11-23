#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <math.h>
#include <iostream>
#include <stdlib.h>
#include <queue>
#include <windows.h>
using namespace std;

// @ clickNum : the click number of the mouse left button
// @ step : use to control witch action should be acted
// @ setText : use to change the color of "Happy New Year!"
// @ upLock : control the card floating
bool upLock = 0;
int clickNum, step, setText;
struct Color { double r, g, b; }textCol[8];
struct Point { double x, y; };

// the base class of quad, triangle, circle, snowflake and text
class Base {
	int mod;
	vector<Color>* c;
	vector<Point>* p;

	void draw_point(vector <Point>* ps, vector <Color>* cs, int mod) {
		glLineWidth(1);
		glBegin(mod);
			vector<Color>::iterator itc = cs->begin();
			vector<Point>::iterator itp = ps->begin();
			for (int i = 0; i < ps->size(); i++) {
				glColor3f((itc + i)->r, (itc + i)->g, (itc + i)->b);
				glVertex2f((itp + i)->x, (itp + i)->y);
			}
		glEnd();
	}

public:
	float rota;
	Point trans, scale;
	Base() { trans = { 0, 0 }; scale = { 1, 1 }; rota = 0; }
	Base(vector<Point>* P, vector<Color>* C, int m) : p(P), c(C), mod(m) { trans = { 0, 0 }; scale = { 1, 1 }; rota = 0;}
	
	void draw_it(bool withBoard) {
		glPushMatrix();
			glTranslatef(trans.x, trans.y, 0);
			glScalef(scale.x, scale.y, 1);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			draw_point(p, c, mod);
			if (withBoard) {
				vector<Color> bs;
				for (int i = 0; i < p->size(); i++) bs.push_back({ 0, 0, 0 });
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				draw_point(p, &bs, mod);
			}
		glPopMatrix();
	}
	void addTrans(Point t) { trans.x += t.x, trans.y += t.y; }
	void setTrans(Point t) { trans.x = t.x, trans.y = t.y; }
	void setScale(Point t) { scale.x = t.x, scale.y = t.y; }
	void addRota(float r) { rota += r; }
};

class Circle: public Base {
	Color c;
	Point p;
	int r, left, right;

	void draw_points(int tp) {
		glLineWidth(2);
		glBegin(tp);
		if (tp == GL_POLYGON) glVertex2f(p.x, p.y);
		for (int i = left; i < right; i++) {
			double x = r * sin(i * (6.284 / 360.0)) + p.x;
			double y = r * cos(i * (6.284 / 360.0)) + p.y;
			glVertex2f(x, y);
		}
		glEnd();
	}

public:
	Circle(int R, Point P, int L, int Ri, Color C) : r(R), p(P), c(C), left(L), right(Ri) {}
	void draw_it(int withBoard) {
		glPushMatrix();
		glTranslatef(trans.x, trans.y, 0);
		glScaled(scale.x, scale.y, 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(c.r, c.g, c.b);
		draw_points(GL_POLYGON);

		if (withBoard) {
			glColor3f(0, 0, 0);
			draw_points(GL_LINE_STRIP);
		}
		glPopMatrix();
	}
};

// calculate the non linear speed of actions
class Degree {
	double start, end, Itr, inter;

public:
	Degree(double s, double b, double i) : start(s), Itr(s), end(b), inter(i) {}
	double getNextSin(bool isLoop) {
		float ans = sin(Itr * (6.284 / 360.0));
		Itr += inter;
		if (isLoop && Itr > end) Itr = start;
		return ans;
	}
	bool isEnd() { return Itr > end ? 1 : 0; }
};

class Snowflake: public Base {
	int depth; // the complexity of the snowflake
	Color c;
	Point p0, p1, p2;
	
	Point cal_new_point(Point a, Point b) {
		Point v = { b.x - a.x , b.y - a.y };
		Point v2 = a;
		v2.x += v.x * cos(60 * (6.284 / 360.0)) - v.y * sin(60 * (6.284 / 360.0));
		v2.y += v.x * sin(60 * (6.284 / 360.0)) + v.y * cos(60 * (6.284 / 360.0));
		return v2;
	}

	void div_line(int depth, Point a, Point b) {
		if (depth == 0) { glVertex2f(b.x, b.y); return; }
		Point v0 = { a.x + (b.x - a.x) / 3, a.y + (b.y - a.y) / 3 };
		Point v2 = { a.x + (b.x - a.x) / 1.5, a.y + (b.y - a.y) / 1.5 };
		Point v1 = cal_new_point(v0, v2);
		// draw the snowflake recursively
		div_line(depth - 1, a, v0);
		div_line(depth - 1, v0, v1);
		div_line(depth - 1, v1, v2);
		div_line(depth - 1, v2, b);
	}

public:
	Snowflake(int dep, Point a, Point b, Color C, bool isRot) : depth(dep), c(C) {
		// @ isRot : the snowflake will rotate or not
		//			 if true, it should be generated at origin point and translate to the wanted position
		// @ a and b : two base point of the snowflake, the third one will be calculated
		Point d = cal_new_point(a, b);
		if (isRot) {
			Point del = { (a.x + b.x + d.x) / 3, (a.y + b.y + d.y) / 3 };
			p0 = { a.x - del.x, a.y - del.y };
			p1 = { b.x - del.x, b.y - del.y };
			p2 = { d.x - del.x, d.y - del.y };
			setTrans(del);
		}
		else { p0 = a; p1 = b; p2 = d; }
	}

	void draw_it() {
		glPushMatrix();
			glTranslatef(trans.x, trans.y, 0);
			glScalef(scale.x, scale.y, 1);
			glRotatef(rota, 0, 0, 1);
			glColor3f(c.r, c.g, c.b);
			glLineWidth(2);
			glBegin(GL_LINE_LOOP);
				div_line(depth, p1, p0);
				div_line(depth, p0, p2);
				div_line(depth, p2, p1);
			glEnd();
		glPopMatrix();
	}
};

class Text: public Base {
	Point p;
	Color c;

	HDC selectFont() {
		HFONT hFont = CreateFontA(scale.x, 0, 0, 0, FW_MEDIUM, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "");
		SelectObject(wglGetCurrentDC(), hFont);
		return wglGetCurrentDC();
	}

	void drawString(const char* str) {
		static  int isFirstCall = 1;
		static GLuint lists;
		if (isFirstCall) {
			isFirstCall = 0;
			lists = glGenLists(128);
			wglUseFontBitmaps(selectFont(), 0, 128, lists);
		}
		glPushAttrib(GL_LIST_BIT);
		while (*str != '\0') glCallList(lists + *str++);
		glPopAttrib();
	}

public:
	Text(Point P, Color C, double s) : p(P), c(C) { scale.x = s; }
	void draw_it(const char *str) {
		glPushMatrix();
			glTranslatef(trans.x, trans.y, 0); 
			glColor3f(c.r, c.b, c.g);
			glRasterPos2f(p.x, p.y);
			drawString(str);
		glPopMatrix();
	}
	void setColor(Color C) { c = C; }
};

class Card {
	vector<Color> c, cakeC, milk;
	vector<Point> pol[4], ps[2];
	Base *quad, *body, *p[4];
	Snowflake *sf[3];
	Circle* cir[2], *fruit;

public:
	Card() {
		for (int i = 0; i < 4; i++) cakeC.push_back({ 1, 0.82, 0.69 });
		c.push_back({ 0.9, 0.99, 1 }); c.push_back({ 1, 1, 1 }); c.push_back({ 0.9, 0.99, 1 }); c.push_back({ 1, 1, 1 });
		ps[0].push_back({ -300, -300 }); ps[0].push_back({ -300, 0 }); ps[0].push_back({ 300, 0 }); ps[0].push_back({ 300, -300 });
		ps[1].push_back({ -70, -220 }); ps[1].push_back({ 70, -220 }); ps[1].push_back({ 70, -120 }); ps[1].push_back({ -70, -120 });
		quad = new Base(&ps[0], &c, GL_QUADS);
		body = new Base(&ps[1], &cakeC, GL_QUADS);
		sf[0] = new Snowflake(3, {-240, -270}, {-180, -200}, { 0, 0.95, 0.88 }, 0);
		sf[1] = new Snowflake(4, { 120, -100 }, { 200, -180 }, { 0, 0.95, 0.88 }, 0);
		sf[2] = new Snowflake(2, { 260, -190 }, { 200, -200 }, { 0, 0.95, 0.88 }, 0);
		cir[0] = new Circle(298, { -299, -1 }, 90, 181, { 0.9, 0.99, 1 });
		cir[1] = new Circle(250, { 299, -299 }, -90, 1, { 0.9, 0.99, 1 });
		fruit = new Circle(10, { 0, -115 }, 0, 361, { 1, 0, 0 });

		double x = -70, y;
		for (int j = 0; j < 4; j++) {
			double k = (-70 + j * 35);
			pol[j].push_back({k, -120});
			Degree deg(0, 180, 1);
			for (int i = 0; i < 181; i++) {
				x = -70 + j * 35 + (double)i * 140.0 / 720.0;
				y = -140 - deg.getNextSin(1) * 30;
				pol[j].push_back({x, y});
			}
			pol[j].push_back({x, -120});
			for (int i=0;i<pol[j].size(); i++) milk.push_back({1, 1, 1});
			p[j] = new Base(&pol[j], &milk, GL_POLYGON);
		}
	}

	void draw_it() {
		quad->draw_it(1);
		for (int i = 0; i < 2; i++) cir[i]->draw_it(0);
		for (int i = 0; i < 3; i++) sf[i]->draw_it();

		fruit->draw_it(0);
		body->draw_it(0);
		for (int i = 0; i < 4; i++) p[i]->draw_it(0);
	}

	// change the transform variable such as trans, scale and rota
	void transform(void (Base:: *func)(Point), Point T) {
		(quad->*func)(T); (body->*func)(T); (fruit->*func)(T);
		for (int i = 0; i < 4; i++)(p[i]->*func)(T);
		for (int i = 0; i < 3; i++) (sf[i]->*func)(T);
		for (int i = 0; i < 2; i++) (cir[i]->*func)(T);
	}
} card;

class Letter {
	vector<Color> quadC, triC;
	Base *quad[2], * tri[3];
	vector<Point> quadP[2], triP[3];

public:
	Letter() {
		quadC.push_back({ 1, 0.84, 0.5 }); quadC.push_back({ 1, 0.84, 0.5 }); quadC.push_back({ 1, 0.84, 0.65 }); quadC.push_back({ 1, 0.84, 0.65 });
		quadP[0].push_back({ 250, 200 }); quadP[0].push_back({ 250, 600 }); quadP[0].push_back({ 950, 600 }); quadP[0].push_back({ 950, 200 });
		quadP[1].push_back({ 450, 400 }); quadP[1].push_back({ 750, 400 }); quadP[1].push_back({ 950, 200 }); quadP[1].push_back({ 250, 200 });
		quad[0] = new Base(&quadP[0], &quadC, GL_QUADS);
		quad[1] = new Base(&quadP[1], &quadC, GL_QUADS);

		triC.push_back({ 1, 0.84, 0.65 }); triC.push_back({ 1, 0.84, 0.65 }); triC.push_back({ 1, 0.84, 0.5 });
		triP[0].push_back({ 250, 200 }); triP[0].push_back({ 250, 600 }); triP[0].push_back({ 450, 400 });
		triP[1].push_back({ 950, 200 }); triP[1].push_back({ 950, 600 }); triP[1].push_back({ 750, 400 });
		triP[2].push_back({ -350, 0 }); triP[2].push_back({ 350, 0 }); triP[2].push_back({ 0, -350 });
		for (int i = 0; i < 3; i++) tri[i] = new Base(&triP[i], &triC, GL_TRIANGLES);
	}

	void draw_it(bool withBoard) {
		quad[0]->draw_it(withBoard);
		if ((clickNum < 2 && step < 1) || step >= 7) {
			if (clickNum == 0)
				card.transform(&Base::setTrans, { 600, 550 });
			card.draw_it();
			quad[1]->draw_it(withBoard);
		}

		if (clickNum == 0) { tri[2]->setTrans({ 600, 600 }); }
		tri[2]->draw_it(withBoard);

		if ((clickNum >= 2 || step >= 1) && step < 7) {
			card.draw_it();
			quad[1]->draw_it(withBoard);
		}
		tri[0]->draw_it(withBoard);
		tri[1]->draw_it(withBoard);
	}
	void setTriangleScale(int id, Point s) { tri[id]->setScale(s); }
	void addAllTrans(Point t) {
		for (int i = 0; i < 2; i++) quad[i]->addTrans(t);
		for (int i = 0; i < 3; i++) tri[i]->addTrans(t);
	}
} letter;

class bg {
	vector<Color> skyC;
	vector<Point> quadP;
	Base *sky;
	Circle *snowman[5], *cloud[5];
	Text *text;

public:
	bg() {
		text = new Text({ 230, 650 }, { 0, 0, 0 }, 100);
		snowman[0] = new Circle(50, { 120, 200 }, -135, 135, { 1, 1, 1 });
		snowman[1] = new Circle(100, { 120, 70 }, 20, 340, { 1, 1, 1 });
		snowman[2] = new Circle(8, { 100, 210 }, 0, 361, { 0, 0, 0 });
		snowman[3] = new Circle(8, { 140, 210 }, 0, 361, { 0, 0, 0 });
		snowman[4] = new Circle(30, { 120, 210 }, 150, 210, { 1, 1, 1 });
		cloud[0] = new Circle(50, { 300, 900 }, -90, 91, { 0.97, 0.97, 0.97 });
		cloud[1] = new Circle(100, { 400, 900 }, -90, 91, { 0.98, 0.98, 0.98 });
		cloud[2] = new Circle(70, { 1000, 1000 }, -90, 91, { 0.96, 0.96, 0.96 });
		cloud[3] = new Circle(150, { 850, 1000 }, -90, 91, { 0.98, 0.98, 0.98 });
		cloud[4] = new Circle(120, { 1150, 770 }, -90, 91, { 0.97, 0.97, 0.97 });
		skyC.push_back({ 1, 1, 1 }); skyC.push_back({ 0.7, 0.94, 1 }); skyC.push_back({ 0.7, 0.94, 1 }); skyC.push_back({ 1, 1, 1 });
		quadP.push_back({ 0, 0 }); quadP.push_back({ 0, 1200 }); quadP.push_back({ 1200, 1200 }); quadP.push_back({ 1200, 0 });
		sky = new Base(&quadP, &skyC, GL_QUADS);
	}

	void draw_it() {
		sky->draw_it(0);
		for (int i = 0; i < 5; i++) {
			snowman[i]->draw_it(1);
			cloud[i]->draw_it(0);
		}
		text->draw_it("Happy New Year!");
	}
	void addAllTrans(Point t) {
		text->addTrans(t);
		for (int i = 0; i < 5; i++) {
			snowman[i]->addTrans(t);
			cloud[i]->addTrans(t);
		}
	}
	void setColor(Color c) { text->setColor(c); }
}b;

static queue<pair <Snowflake, Point>> que;
void snow() {
	int r = rand() % 10;
	if (r >= 9) { // add new snowflake
		int d = rand() % 4 + 1;
		Point a = { rand() % 2400, 1200};
		Point b = { a.x + rand() % 50 + 10, 1200 };
		Point dir = { -rand() % 4 - 3, -rand() % 7 - 3}; // direction and speed
		Snowflake s(d, a, b, { 0, 0.95, 0.88 }, 1);
		que.push(make_pair(s, dir));
	}

	int size = que.size();
	for (int i = 0; i < size; i++) { // move snowflakes. If one moves out of the window, delete it.
		Snowflake s = que.front().first;
		Point p = que.front().second;
		if (s.trans.x >= -50 && s.trans.y >= -50) {
			s.addTrans(p);
			s.addRota(1);
			que.push(make_pair(s, p));
		}
		que.pop();
	}
}

void card_float() {
	static Degree cardD(0, 180, 2);
	double deltaY = cardD.getNextSin(1) / 2;
	if (!upLock && deltaY == 0) upLock = 1;
	else if (upLock) {
		deltaY = -deltaY;
		if (deltaY == 0) upLock = 0;
	}
	card.transform(&Base::addTrans, { 0, deltaY });
}

void OnTimer(int value) {
	// change the integer into rgb by changing it into 3 bit binary number: e.g. setText = 5 --> (Color){1, 0, 1}
	setText = (setText + 7) % 7;
	b.setColor({ (double)(setText % 2), (double)((setText / 2) % 2), (double)((setText / 4) % 2) });

	double S = 0;
	if (clickNum >= 1 && (step == 0 || step == 1)) {
		static Degree d(-90, 90, 2);
		S = -d.getNextSin(0);
		if (d.isEnd()) step = 2;
		else letter.setTriangleScale(2, { 1, S });
	}
	if (S < 0 && (step == 0 || step == 1)) {
		step = 1;
		static Degree scardD(90, 180, 2);
		double deltaY = scardD.getNextSin(0) * 4.5;
		if (scardD.isEnd()) deltaY = 0;
		card.transform(&Base::addTrans, { 0, deltaY });
	}
	if (step == 2 && clickNum >= 1) card_float();
	if (clickNum >= 2 && step == 2) {
		static Degree letterD(0, 180, 2);
		static Degree cardD(0, 180, 2);
		static Degree backD(0, 90, 1);
		double deltaT = -letterD.getNextSin(0) * 20;
		double deltaD = cardD.getNextSin(0) * 9;
		double deltaB = -backD.getNextSin(0) * 5;
		if (backD.isEnd()) deltaB = 0;
		if (letterD.isEnd()) deltaT = 0;
		if (cardD.isEnd()) {
			step = 3;
			deltaD = 0;
		}
		letter.addAllTrans({ 0, deltaT });
		card.transform(&Base::addTrans, { 0, deltaD });
		b.addAllTrans({ 0, deltaB });
	}
	if (step == 3) {
		static Degree cardD(90, 180, 1);
		static Degree cardS(0, 90, 1);
		double deltaD = -cardD.getNextSin(0) * 4;
		double deltaS = (float)min(cardS.getNextSin(0) + 1.0, 1.5);
		if (cardS.isEnd()) deltaS = 1.5;
		if (cardD.isEnd()) deltaD = 0;
		if (cardD.isEnd() && cardS.isEnd()) {
			step = 4;
			upLock = 1;
		}
		card.transform(&Base::addTrans, { 0, deltaD });
		card.transform(&Base::setScale, { deltaS, deltaS });
	}
	if (step == 4 && clickNum >= 2) card_float();
	if (clickNum >= 3 && step == 4) {
		static float S = 1.5;
		static Degree cardD(90, 180, 3);
		double deltaD = cardD.getNextSin(0) * 8;
		double deltaS = S;
		S = (float)max(S - 0.035, 1.0);
		if (S <= 1) deltaS = 1;
		if (cardD.isEnd()) deltaD = 1;
		if (cardD.isEnd() && S <= 1) step = 5;
		card.transform(&Base::addTrans, { 0, deltaD });
		card.transform(&Base::setScale, { deltaS, deltaS });
	}
	if (step == 5) {
		static Degree letterD(90, 180, 1);
		static Degree cardD(0, 180, 4);
		static Degree backD(0, 90, 1);
		double deltaT = letterD.getNextSin(0) * 28;
		double deltaD = -cardD.getNextSin(0) * 19;
		double deltaB = backD.getNextSin(0) * 16;
		if (backD.isEnd()) deltaB = 0;
		if (letterD.isEnd()) deltaT = 0;
		if (cardD.isEnd()) {
			step = 6;
			deltaD = 0;
		}
		letter.addAllTrans({ 0, deltaT });
		card.transform(&Base::addTrans, { 0, deltaD });
		b.addAllTrans({ 0, deltaB });
	}
	if (step == 6) {
		static Degree letterDown(90, 180, 5);
		double deltaD = -letterDown.getNextSin(0) * 5;
		if (letterDown.isEnd()) {
			deltaD = 0;
			step = 7;
		}
		letter.addAllTrans({ 0, deltaD });
		card.transform(&Base::addTrans, { 0, deltaD });
	}
	if (step == 7) {
		static Degree letterDown(90, 180, 5);
		double deltaD = letterDown.getNextSin(0) * 5;
		if (letterDown.isEnd()) {
			deltaD = 0;
			step = 8;
		}
		letter.addAllTrans({ 0, deltaD });
		card.transform(&Base::addTrans, { 0, deltaD });
	}
	if (step == 6 || step == 7 || step == 8) {
		static Degree d(-90, 90, 4);
		double deltaS = d.getNextSin(0);
		if (d.isEnd()) step = 9;
		else letter.setTriangleScale(2, { 1, deltaS });
	}
	snow();
	glutTimerFunc(10, OnTimer, 1);
}

void move(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON)
		clickNum++;
}

void kbfunc(unsigned char key, int x, int y) {
	if (key == 'a' or key == 'A')		setText--;
	else if (key == 'd' or key == 'D')	setText++;
}

void when_in_mainloop() { glutPostRedisplay(); }
void display() {
	glLoadIdentity();
	gluOrtho2D(0, 1200, 0, 1200);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	b.draw_it();
	int size = que.size();
	for (int i = 0; i < size; i++) { // draw snowflakes
		Snowflake s = que.front().first;
		Point p = que.front().second;
		s.draw_it();
		que.push(make_pair(s, p));
		que.pop();
	}
	letter.draw_it(1);
	glutSwapBuffers();
}

int main(int argc, char** argv) {
	srand((int)time(0));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(1200, 1200);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("New Year greeting card");
	glutDisplayFunc(display);
	glutIdleFunc(when_in_mainloop);
	glutTimerFunc(10, OnTimer, 1);
	glutMouseFunc(move);
	glutKeyboardFunc(kbfunc);
	glutMainLoop();
}