#include "connector.hh"
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sstream>

template <typename T>
std::string NumberToString ( T Number )
{
  std::ostringstream ss;
  ss << Number;
    return ss.str();
}

namespace Profiling {

  Node::Node(int sid, int pid, int alt, int kids, NodeStatus status, Connector& c) : _c(c) {

    _node.set_type(message::Node::NODE);

    _node.set_sid(sid);
    _node.set_pid(pid);
    _node.set_alt(alt);
    _node.set_kids(kids);
    _node.set_status(static_cast<message::Node::NodeStatus>(status));
    _node.set_thread_id(-1); /// -1 is default for thread id
  }

  void Node::send() {
    this->_c.sendNode(*this);
  }

  Node::~Node() {

  }

}

namespace Profiling {

  Connector::Connector(unsigned int port, unsigned int tid)
    : port(port), _thread_id(tid),
      socket(io_service)
  {
      int linger = -1;
      connected = false;
    // nanosocket.setsockopt(NN_SOL_SOCKET, NN_LINGER, &linger, sizeof(linger));
    // begin_time = system_clock::now();
  }

  void Connector::sendOverSocket(const message::Node &msg) {
      bool printNodes = true;
      if (printNodes) {
          std::cerr << msg.type() << ","
                    << msg.sid() << ","
                    << msg.restart_id() << ","
                    << msg.pid() << ","
                    << msg.alt() << ","
                    << msg.kids() << ","
                    << msg.status() << ","
                    << msg.time() << ","
                    << msg.label() << ","
                    << msg.nogood() << ","
                    << msg.info() << "\n";
      }
      if (!connected) return;
    std::string msg_str;
    msg.SerializeToString(&msg_str);

    void *buf = new char[msg_str.size()];
    memcpy(buf, msg_str.c_str(), msg_str.size());

    // Sometimes sending will fail with EINTR.  In this case, we try to
    // send the message again.
    while (true) {
      int failed_attempts = 0;
      // try {
          // int bytes = write(socket, buf, msg_str.size());

      uint32_t bufSize = msg_str.size();
      // std::cout << "sending message size: " << bufSize << "\n";
      socket.write_some(asio::buffer(reinterpret_cast<void*>(&bufSize), sizeof(bufSize)));

//      usleep(1000000);

      // std::cout << "sending message\n";
      socket.write_some(asio::buffer(buf, bufSize));
      
//      usleep(1000000);
        // int bytes = nanosocket.send(buf, msg_str.size(), 0);
        // if (bytes == -1) {
        //   failed_attempts++;
        //   if (failed_attempts > 10) abort();
        //   continue;
        // }
        // if (bytes < msg_str.size())
        //     abort();
        // Success: stop the loop.
        break;
      // } catch (nn::exception &e) {
      //   failed_attempts++;
      //   if (failed_attempts > 10) abort();
      //   if (e.num() == EINTR) {
      //     continue;
      //   }
      //   // If it was something other than EINTR, rethrow the exception.
      //   throw e;
      // }
    }
  }

  void Connector::sendNode(int sid, int pid, int alt, int kids,
                           NodeStatus status, const char* label, unsigned int thread, int restart,
                           float domain, const std::string& nogood, const std::string& info) {
      if (!connected) return;
    // current_time = system_clock::now();
    // unsigned long long timestamp = static_cast<long long>(duration_cast<microseconds>(current_time - begin_time).count());

    message::Node node;

    node.set_type(message::Node::NODE);

    node.set_sid(sid);
    node.set_pid(pid);
    node.set_alt(alt);
    node.set_kids(kids);

    node.set_status(static_cast<message::Node::NodeStatus>(status));
    node.set_label(label);
    node.set_thread_id(thread);
    node.set_restart_id(restart);
    node.set_domain_size(domain);
    // node.set_solution(solution);
    node.set_nogood(nogood);
    node.set_info(info);

    sendOverSocket(node);
  }

  Node Connector::createNode(int sid, int pid, int alt, int kids, NodeStatus status) {
    return Node(sid, pid, alt, kids, status, *this);
  }

  void Connector::sendNode(const Profiling::Node& node) {
    sendOverSocket(node.get_node());
  }

  void Connector::restart(const std::string& file_path, int restart_id) {
    // std::cerr << "restarting gist, restart_id: " << restart_id << "\n";

    message::Node dummy_node;
    dummy_node.set_type(message::Node::START);

    /// should it be restart_id instead?
    dummy_node.set_restart_id(restart_id);

    /// extract fzn file name
    std::string name(file_path);
    int pos = name.find_last_of('/');
    if (pos > 0) {
      name = name.substr(pos + 1, name.length() - pos - 1);
    }

    dummy_node.set_label(name);

    sendOverSocket(dummy_node);
  }

  void Connector::connect() {
    // std::string address = "tcp://localhost:" + NumberToString(port);
    // endpoint = nanosocket.connect(address.c_str());
    // std::cout << "sending over port: " << port << "\n";
      // socketfd = socket(AF_INET, SOCK_STREAM, 6);
      // connect(socketfd,
      try {
          tcp::resolver resolver(io_service);
          tcp::endpoint endpoint(address::from_string("127.0.0.1"), 1234);
          socket.connect(endpoint);
          connected = true;
      } catch (asio::system_error& e) {
          std::cerr << "couldn't connect to profiler; running solo\n";
          connected = false;
      }
  }

  void Connector::done() {
    message::Node dummy_node;
    dummy_node.set_type(message::Node::DONE);
    sendOverSocket(dummy_node);
  }

  void Connector::disconnect() {
    // nanosocket.shutdown(endpoint);
  }

}
