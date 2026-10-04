#include <memory>
#include <iostream>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <SFML/Graphics.hpp>

#include "../ping-pong-environment.h"

#define RASTER_SIZE 200   // It is for the range [0, 1] while the whole picture range is [-1, 1].
#define SPOT_HALFSIZE_PIXEL 10

using namespace std;
using namespace boost::interprocess;

class EnvironmentState
{
	unique_ptr<shared_memory_object> shm;
	unique_ptr<mapped_region>        region;
public:
	volatile pair<float, float> *pprr_Ball;
	volatile float              *prRacket = NULL;
	EnvironmentState()
	{
		try {
			//Create a shared memory object.
			shm.reset(new shared_memory_object(open_or_create, ENVIRONMENT_STATE_SHARED_MEMORY_NAME, read_write));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " created\n";
			//Set size
			shm->truncate(sizeof(pair<pair<float, float>, float>));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " size set\n";

			//Map the whole shared memory in this process
			region.reset(new mapped_region(*shm, read_only));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " mapped\n";
			pprr_Ball = (volatile pair<float, float> *)region->get_address();
			prRacket = &((volatile pair<pair<float, float>, float> *)pprr_Ball)->second;
		} catch (...) {
			cout << "Cannot access " ENVIRONMENT_STATE_SHARED_MEMORY_NAME "\n";
			exit(-1);
		}
	}
	~EnvironmentState() {shared_memory_object::remove(ENVIRONMENT_STATE_SHARED_MEMORY_NAME);}
};

inline float rPixelX(float rPhysical) {return (rPhysical + 0.5F) * RASTER_SIZE;}
inline float rPixelY(float rPhysical) {return (0.5F - rPhysical) * RASTER_SIZE;}

int main(int ARGC, char *ARGV[])
{
	EnvironmentState es;
	cout << "ping-pong data can be accessed\n";
	// Create the main window
	sf::RenderWindow window(sf::VideoMode(RASTER_SIZE, RASTER_SIZE), "ping-pong", sf::Style::Close);
	sf::CircleShape circle;
	circle.setRadius(SPOT_HALFSIZE_PIXEL);
	circle.setFillColor(sf::Color::White);
	circle.setOrigin(SPOT_HALFSIZE_PIXEL, SPOT_HALFSIZE_PIXEL);
	sf::RectangleShape rectangle;
	rectangle.setSize(sf::Vector2f(3, RACKET_SIZE * RASTER_SIZE));
	rectangle.setOrigin(0, RACKET_SIZE / 2 * RASTER_SIZE);
	rectangle.setFillColor(sf::Color::White);

	// Start the game loop
	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			// Close window: exit
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Clear screen
		window.clear();
		circle.setPosition(rPixelX(es.pprr_Ball->first), rPixelY(es.pprr_Ball->second));
		window.draw(circle);
		rectangle.setPosition(0, rPixelY(*es.prRacket));
		window.draw(rectangle);

		// Update the window
		window.display();
	}
	return EXIT_SUCCESS;
}
