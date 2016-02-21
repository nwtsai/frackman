#include "StudentWorld.h"
#include <string>
#include <vector>
#include <cstdlib>
#include "Actor.h"
#include "GameController.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include "GraphObject.h"

using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

StudentWorld::StudentWorld(std::string assetDir)
	: GameWorld(assetDir)
{
	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 60; j++)
		{
			m_dirt[i][j] = nullptr;
		}
	}
	m_barrelsLeft = min(2 + getLevel(), 20);
	fracker = nullptr;
}

StudentWorld::~StudentWorld()
{
	for (int i = 0; i < m_actors.size(); i++)
		delete m_actors.at(i);
	m_actors.clear();

	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 60; y++)
		{
			if (m_dirt[x][y] != nullptr)
			{
				delete m_dirt[x][y];
				m_dirt[x][y] = nullptr;
			}
				
		}
	}

	delete fracker;
}

int StudentWorld::init()
{
	// initialize the dirt
	for (int r = 0; r < 64; r++)
	{
		for (int c = 0; c < 64; c++)
		{
			if ((r < 30 || r > 33 || c < 4) && c < 60)
			{
				m_dirt[r][c] = new Dirt(r, c);
			}
			else
			{
				m_dirt[r][c] = nullptr;
			}
		}
	}

	// initialize the FrackMan
	fracker = new FrackMan(this);

	// initialize the Boulders
	addBoulders();

	// initialize the Gold Nuggets
	addNuggets();

	// initialize the Barrels
	addBarrels();

	// add a Protester
	addProtester();

	return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
	// update status text
	setDisplayText();

	// ask every object to do something
	fracker->doSomething();
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors.at(i)->isStillAlive())
		{
			m_actors.at(i)->doSomething();

			if (!fracker->isStillAlive())
			{
				decLives();
				return GWSTATUS_PLAYER_DIED;
			}

			if (m_barrelsLeft == 0)
			{
				GameController::getInstance().playSound(SOUND_FINISHED_LEVEL);
				advanceToNextLevel();
				return GWSTATUS_FINISHED_LEVEL;
			}
		}
	}

	// delete actors that need to be deleted and remove them from the vector
	removeDeadObjects();

	int G = getLevel() * 25 + 300;
	if (randInt(1, G) == 1)
	{
		if (randInt(1, 5) == 1)
		{
			addSonarKit();
		}
		else
		{
			addWaterPool();
		}
	}

	return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
	for (int i = 0; i < m_actors.size(); i++)
		delete m_actors.at(i);
	m_actors.clear();

	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 60; y++)
		{
			if (m_dirt[x][y] != nullptr)
			{
				delete m_dirt[x][y];
				m_dirt[x][y] = nullptr;
			}
		}
	}

	delete fracker;
}

bool StudentWorld::isCloseTo(int x, int y, Actor* obj)
{
	return isWithinRadius(x, y, obj->getX(), obj->getY(), 6.0);
}

bool StudentWorld::isCloseToAnyActor(int x, int y)
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		if (isCloseTo(x, y, *p))
		{
			return true;
		}
		p++;
	}
	return false;
}

bool StudentWorld::isCollidingWith(int x, int y, Actor* obj)
{
	return isWithinRadius(x, y, obj->getX(), obj->getY(), 3.0);
}

bool StudentWorld::canMoveHere(int x, int y)
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		if ((*p)->doesThisBlock() == true && isCollidingWith(x, y, *p))
		{
			return false;
		}
		p++;
	}
	return true;
}

bool StudentWorld::isThereDirtHere(int x, int y)
{
	if (x < 0 || y < 0 || x > 63 || y > 63)
		return false;

	if (m_dirt[x][y] != nullptr)
		return true;
	return false;
}

bool StudentWorld::isThereDirtInThisBox(int x, int y)
{
	if (x < 0 || y < 0 || x > 60 || y > 60)
		return false;

	for (int i = x; i < x + 4; i++)
	{
		for (int j = y; j < y + 4; j++)
		{
			if (m_dirt[i][j] != nullptr)
				return true;
		}
	}
	return false;
}

bool StudentWorld::isThereBoulderInThisBox(int x, int y)
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		// if the actor is a Boulder
		if ((*p)->doesThisBlock())
		{
			for (int i = x - 3; i < x + 4; i++)
			{
				for (int j = y - 3; j < y + 4; j++)
				{
					if (i == (*p)->getX() && j == (*p)->getY())
						return true;
				}
			}
		}
		p++;
	}
	return false;
}

bool StudentWorld::isCollidingWithBoulder(int x, int y)
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		// if the actor is a Boulder
		if ((*p)->doesThisBlock())
		{
			if (isCollidingWith(x, y, *p))
				return true;
		}
		p++;
	}
	return false;
}

bool StudentWorld::isWithinShoutingDistance(int x, int y)
{
	if (isWithinRadius(x, y, fracker->getX(), fracker->getY(), 4.0))
		return true;
	return false;
}

bool StudentWorld::isFacingFrackMan(int x, int y, GraphObject::Direction dir)
{
	int fx = fracker->getX();
	int fy = fracker->getY();

	/*if (x < fx && dir == right)
		return true;
	else if (x > fx && dir == left)
		return true;
	else if (y < fy && dir == up)
		return true;
	else if (y > fy && dir == down)
		return true;*/

	return true;
}

bool StudentWorld::isInLineOfSight(int x, int y)
{
	int fx = fracker->getX();
	int fy = fracker->getY();

	for (int i = x - 3; i < x + 4; i++)
	{
		for (int j = y - 3; j < y + 4; j++)
		{
			if (i == fx || j == fy)
				return true;
		}
	}
	return false;
}

void StudentWorld::annoyFrackMan()
{
	fracker->reduceHP(2);
}

void StudentWorld::destroyDirt(int x, int y)
{
	for (int i = x; i <= x + 3; i++)
	{
		for (int j = y; j <= y + 3; j++)
		{
			if (m_dirt[i][j] != nullptr && j < 60 && isThereDirtHere(i, j))
			{
				m_dirt[i][j]->setVisible(false);
				playSound(SOUND_DIG);
				delete m_dirt[i][j];
				m_dirt[i][j] = nullptr;
			}
		}
	}
}

void StudentWorld::removeDeadObjects()
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		if ((*p)->isStillAlive() == false)
		{
			vector<Actor*>::iterator temp = p;
			delete *p;
			p = m_actors.erase(temp);
		}
		else
		{
			p++;
		}
	}
}

bool StudentWorld::isBlocked(int x, int y)
{
	if (isThereDirtInThisBox(x, y) || isCollidingWithBoulder(x, y))
		return true;
	return false;
}

void StudentWorld::findBestPathFromTopRight()
{
	// findPath(60, 60, );
}

bool StudentWorld::findPath(int x, int y, GraphObject::Direction dir)
{
	GraphObject::Direction maze[60][60];

	if (!isBlocked(x, y))
	{
		maze[x][y] = dir;
	}
	else
	{
		// maze[x][y] = right;
	}
	return false; // change this

	// if there is any dirt or boulders within the box starting with x,y in the left bottom corner
	
}

void StudentWorld::addBoulders()
{
	int numBoulders = max(getLevel() / 2 + 2, 6);
	for (int boul = 0; boul < numBoulders; boul++)
	{
		int randX = randInt(0, 60);
		int randY = randInt(20, 56);
		
		/*for (int a = 0; a < m_actors.size(); a++)
		{
			while (isCollidingWith(randX, randY, m_actors.at(a)) || (randX >= 26 && randX <= 33))
			{
				randX = randInt(0, 60);
				randY = randInt(20, 56);
			}
		}*/

		// If it is the first Actor, check if it is in mine shaft
		while (randX >= 26 && randX <= 33)
		{
			randX = randInt(0, 60);
		}

		vector<Actor*>::iterator p = m_actors.begin();
		while (p != m_actors.end())
		{
			if (isCloseToAnyActor(randX, randY) || (randX >= 26 && randX <= 33))
			{
				randX = randInt(0, 60);
				randY = randInt(20, 56);
			}
			else
			{
				p++;
			}
		}

		m_actors.push_back(new Boulder(randX, randY, this, fracker));
	}
}

void StudentWorld::addNuggets()
{
	int numNugs = min(5 - getLevel() / 2, 2);
	for (int nugs = 0; nugs < numNugs; nugs++)
	{
		int randX = randInt(0, 60);
		int randY = randInt(20, 56);

		// If it is the first Actor, check if it is in mine shaft
		while (randX >= 26 && randX <= 33)
		{
			randX = randInt(0, 60);
		}

		vector<Actor*>::iterator p = m_actors.begin();
		while (p != m_actors.end())
		{
			if (isCloseToAnyActor(randX, randY) || (randX >= 26 && randX <= 33))
			{
				randX = randInt(0, 60);
				randY = randInt(20, 56);
			}
			else
			{
				p++;
			}
		}

		m_actors.push_back(new GoldNugget(randX, randY, true, this, fracker));
	}
}

void StudentWorld::dropNugget(int x, int y)
{
	m_actors.push_back(new GoldNugget(x, y, false, this, fracker));
}

void StudentWorld::addBarrels()
{
	int numBarrels = min(2 + getLevel(), 20);
	for (int barrels = 0; barrels < numBarrels; barrels++)
	{
		int randX = randInt(0, 60);
		int randY = randInt(20, 56);

		// If it is the first Actor, check if it is in mine shaft
		while (randX >= 26 && randX <= 33)
		{
			randX = randInt(0, 60);
		}

		vector<Actor*>::iterator p = m_actors.begin();
		while (p != m_actors.end())
		{
			if (isCloseToAnyActor(randX, randY) || (randX >= 26 && randX <= 33))
			{
				randX = randInt(0, 60);
				randY = randInt(20, 56);
			}
			else
			{
				p++;
			}
		}

		m_actors.push_back(new Barrel(randX, randY, this, fracker));
	}
}

void StudentWorld::addSonarKit()
{
	SonarKit* sk = new SonarKit(0, 60, this, fracker);
	m_actors.push_back(sk);
}

void StudentWorld::addWaterPool()
{
	int randX = randInt(0, 60);
	int randY = randInt(0, 60);

	while (isThereDirtInThisBox(randX, randY))
	{
		randX = randInt(0, 60);
		randY = randInt(0, 60);
	}

	WaterPool* water = new WaterPool(randX, randY, this, fracker);
	m_actors.push_back(water);
}

void StudentWorld::insertSquirt(int x, int y, GraphObject::Direction dir)
{
	if (dir == GraphObject::left && isThereDirtInThisBox(x - 3, y) == false)
		m_actors.push_back(new Squirt(x - 3, y, dir, this, fracker));
	else if (dir == GraphObject::right && isThereDirtInThisBox(x + 3, y) == false)
		m_actors.push_back(new Squirt(x + 3, y, dir, this, fracker));
	else if (dir == GraphObject::up && isThereDirtInThisBox(x, y + 3) == false)
		m_actors.push_back(new Squirt(x, y + 3, dir, this, fracker));
	else if (dir == GraphObject::down && isThereDirtInThisBox(x, y - 3) == false)
		m_actors.push_back(new Squirt(x, y - 3, dir, this, fracker));
}

void StudentWorld::addProtester()
{
	Protester* pro = new Protester(this);
	m_actors.push_back(pro);
}

int StudentWorld::min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

int StudentWorld::max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

int StudentWorld::randInt(int l, int h)
{
	if (h < l)
		swap(h, l); 
	return l + (rand() % (h - l + 1)); 
}

void StudentWorld::setDisplayText()
{
	int score = getScore();
	int level = getLevel();
	int lives = getLives();
	int health = fracker->getHP();
	int squirts = fracker->getSquirts();
	int gold = fracker->getGold();
	int sonar = fracker->getsCharges();
	int barrelsLeft = m_barrelsLeft;

	string s = formatDisplayText(score, level, lives, health, squirts, gold, sonar, barrelsLeft);
	setGameStatText(s);
}

string StudentWorld::formatDisplayText(int score, int level, int lives, int health, int squirts, int gold, int sonar, int barrelsLeft)
{
	stringstream sa;
	string scr = "Scr: ";
	sa << setw(6) << setfill('0') << score;
	string a = sa.str();

	stringstream sb;
	string lvl = "Lvl: ";
	sb << setw(2) << setfill(' ') << level;
	string b = sb.str();

	stringstream sc;
	string lvs = "Lives: ";
	sc << setw(1) << setfill(' ') << lives;
	string c = sc.str();

	stringstream sd;
	string HP = "Hlth: ";
	sd << setw(3) << setfill(' ') << health;
	string d = sd.str();

	stringstream se;
	string wtr = "Water: ";
	se << setw(2) << setfill(' ') << squirts;
	string e = se.str();

	stringstream sf;
	string gld = "Gld: ";
	sf << setw(2) << setfill(' ') << gold;
	string f = sf.str();

	stringstream sg;
	string snr = "Sonar: ";
	sg << setw(2) << setfill(' ') << sonar;
	string g = sg.str();

	stringstream sh;
	string oil = "Oil Left: ";
	sh << setw(2) << setfill(' ') << barrelsLeft;
	string h = sh.str();

	return scr + a + "  " + lvl + b + "  " + lvs + c + "  " + HP + d + "%  " + wtr + e + "  " + gld + f + "  " + snr + g + "  " + oil + h;
}

bool StudentWorld::isWithinRadius(int x1, int y1, int x2, int y2, double r)
{
	double xDiff = x1 - x2;
	double yDiff = y1 - y2;
	double dist = sqrt(pow(xDiff, 2) + pow(yDiff, 2));
	if (dist <= r)
		return true;
	return false;
}

void StudentWorld::reduceBarrels()
{
	m_barrelsLeft--;
}

FrackMan* StudentWorld::getFrack()
{
	return fracker;
}

void StudentWorld::revealCloseObjects(int x, int y)
{
	vector<Actor*>::iterator p = m_actors.begin();
	while (p != m_actors.end())
	{
		if (isWithinRadius(x, y, (*p)->getX(), (*p)->getY(), 12.0))
		{
			(*p)->setVisible(true);
		}
		p++;
	}
}