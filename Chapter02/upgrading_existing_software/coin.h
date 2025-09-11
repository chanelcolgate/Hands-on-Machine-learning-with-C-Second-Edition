#include <iostream>

using namespace std;
class Coin {
	int state;
	int x,y,value;
	enum states {COLLECTED, AVAILABLE};
	public:
	Coin() {
		x = rand()%9+8;
		y = rand()%9+8;
		value = rand()%100;
		state = AVAILABLE;
	}

	int getX() {
		return x;
	}

	int getY() {
		return y;
	}

	int getValue() {
		return value;
	}

	void setValue(int ival) {
		value = ival;
	}

	int getState() {
		return state;
	}

	void setState(int istate) {
		state = istate;
	}

	friend bool operator==(Coin &c1, Coin &c2);
	friend ostream& operator<<(ostream &o, Coin &c);
};

bool operator==(Coin &c1, Coin &c2) {
	if (c1.getValue() == c2.getValue()) {
		return true;
	}
	return false;
}

ostream& operator<<(ostream &o, Coin &c) {
	o<<c.getValue();
	return o;
}
