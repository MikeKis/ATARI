#include <memory>
#include <iostream>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <SFML/Graphics.hpp>

// .specs/2026-10-13_ping-pong-imitation-learning.md
#include <cstring>

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
	volatile SharedEnvironmentState *pst = NULL;
	EnvironmentState()
	{
		try {
			shm.reset(new shared_memory_object(open_or_create, ENVIRONMENT_STATE_SHARED_MEMORY_NAME, read_write));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " created\n";
			shm->truncate(sizeof(SharedEnvironmentState));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " size set\n";

			region.reset(new mapped_region(*shm, read_write));
			cout << ENVIRONMENT_STATE_SHARED_MEMORY_NAME " mapped\n";
			pst = (volatile SharedEnvironmentState *)region->get_address();
			memset((void *)pst, 0, sizeof(SharedEnvironmentState));
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
		float rSize = es.pst->racket_size > 0.F ? es.pst->racket_size : DEFAULT_RACKET_SIZE;
		rectangle.setSize(sf::Vector2f(3, rSize * RASTER_SIZE));
		rectangle.setOrigin(0, rSize / 2 * RASTER_SIZE);
		circle.setPosition(rPixelX(es.pst->x), rPixelY(es.pst->y));
		window.draw(circle);
		rectangle.setPosition(0, rPixelY(es.pst->racket_y));
		window.draw(rectangle);

		// Update the window
		window.display();
	}
	return EXIT_SUCCESS;
}
