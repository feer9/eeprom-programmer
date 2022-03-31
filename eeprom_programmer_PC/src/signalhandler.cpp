#include "signalhandler.h"
#include <assert.h>
#include <set>

#include <windows.h>

// There can be only ONE SignalHandler per process
SignalHandler* g_handler(NULL);

BOOL WINAPI WIN32_handleFunc(DWORD);
int WIN32_physicalToLogical(DWORD);
DWORD WIN32_logicalToPhysical(int);
std::set<int> g_registry;

SignalHandler::SignalHandler(int mask) : _mask(mask)
{
	assert(g_handler == NULL);
	g_handler = this;

	SetConsoleCtrlHandler(WIN32_handleFunc, TRUE);

	for (int i=0;i<numSignals;i++)
	{
		int logical = 0x1 << i;
		if (_mask & logical)
		{
			g_registry.insert(logical);
		}
	}

}

SignalHandler::~SignalHandler()
{
	SetConsoleCtrlHandler(WIN32_handleFunc, FALSE);
}

DWORD WIN32_logicalToPhysical(int signal)
{
	switch (signal)
	{
	case SignalHandler::SIG_INT: return CTRL_C_EVENT;
	case SignalHandler::SIG_TERM: return CTRL_BREAK_EVENT;
	case SignalHandler::SIG_CLOSE: return CTRL_CLOSE_EVENT;
	default:
		return ~(unsigned int)0; // SIG_ERR = -1
	}
}

int WIN32_physicalToLogical(DWORD signal)
{
	switch (signal)
	{
	case CTRL_C_EVENT: return SignalHandler::SIG_INT;
	case CTRL_BREAK_EVENT: return SignalHandler::SIG_TERM;
	case CTRL_CLOSE_EVENT: return SignalHandler::SIG_CLOSE;
	default:
		return SignalHandler::SIG_UNHANDLED;
	}
}

BOOL WINAPI WIN32_handleFunc(DWORD signal)
{
	if (g_handler)
	{
		int signo = WIN32_physicalToLogical(signal);
		// The std::set is thread-safe in const reading access and we never
		// write to it after the program has started so we don't need to
		// protect this search by a mutex
		std::set<int>::const_iterator found = g_registry.find(signo);
		if (signo != -1 && found != g_registry.end())
		{
			return g_handler->handleSignal(signo) ? TRUE : FALSE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
}
