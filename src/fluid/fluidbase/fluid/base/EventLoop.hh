/** @file */
#ifndef __EVENTLOOP_HH__
#define __EVENTLOOP_HH__

namespace fluid_base {

class BaseOFServer;
class BaseOFConnection;

/**
A EventLoop runs an event loop for connections. It will activate the callbacks
associated with them. The class using an EventLoop should tipically assign
incoming connections in a round-robin fashion.

There might be more than one event loop in use in applications. In this case,
each EventLoop can be run in a thread.
*/
// An EventLoop is pretty much a simple wrapper around libevent's event_base
class EventLoop {
 public:
  /**
  Create a EventLoop.

  @param id event loop id
  */
  EventLoop(int id);
  ~EventLoop();

  /**
  Run this event loop (which will block the calling thread). When
  EventLoop::stop is called, this method will unblock, run the callbacks
  of pending events and return.

  Calling EventLoop::stop first will prevent this method from running. */
  void run();

  /**
  Force the event loop to stop. It will finish running the current event
  callback and then force EventLoop::run to continue its flow (deal with
  remaining events and quit).

  Calling this method first will prevent EventLoop::run from running. */
  void stop();

  /**
  This method is just an adapter for passing the EventLoop::run method to
  pthread_create. */
  static void* thread_adapter(void* arg);

 private:
  int id;
  bool stopped;

  friend class BaseOFServer;
  friend class BaseOFConnection;
  void* get_base();

  class LibEventEventLoop;
  friend class LibEventEventLoop;
  LibEventEventLoop* m_implementation;
};

}  // namespace fluid_base

#endif