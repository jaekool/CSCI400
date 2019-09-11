// Template singleton
#ifndef SINGLETON_H
#define SINGLETON_H

template <class T>
class Singleton
{
public:
	static T& getInstance() {
		static T _instance;
		return _instance;
	}
private:
	Singleton();          // ctor hidden
	~Singleton();          // dtor hidden
	Singleton(Singleton const&);    // copy ctor hidden
	Singleton& operator=(Singleton const&);  // assign op hidden
};

#endif /* SINGLETON_H */
