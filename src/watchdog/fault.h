#include <signal.h>

#define ENV_FAULT_LOGGER_PID "__FAULT_LOGGER_PID"
class CFaultHandler {
	private:
		static void Handler(int, siginfo_t *, void *);
		static CFaultHandler Instance;
		static int DogPid;
		CFaultHandler(void);
	public:
		static void Initialize(int);

};

