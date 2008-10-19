// usbmux.hpp

#ifndef USBMUX_HPP
#define USBMUX_HPP

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/logic/tribool.hpp>

namespace usbmux {

   enum {
      usbmux_result  = 1,
      usbmux_connect = 2,
      usbmux_hello   = 3,
   };

   struct usbmux_header {
      public:
         boost::uint32_t length;        // length of message, including header
         boost::uint32_t reserved;      // always zero
         boost::uint32_t type;          // message type
         boost::uint32_t tag;           // responses to this query will echo back this tag
   };

   struct usbmux_response {
      public:
         struct usbmux_header header;
         boost::uint32_t result;        // result code of last request
   };
   
   struct usbmux_hello_request {
      public:
         struct usbmux_header header;
   };

   struct usbmux_connect_request {
      public:
         struct usbmux_header header;
         boost::uint32_t  device_id;    // Device ID - Is always 0x19
         boost::uint16_t  dst_port;     // TCP port number
         boost::uint16_t  reserved;
   };

   //

   class proxy_session : public boost::enable_shared_from_this<proxy_session>
   {
      public:
         
         proxy_session(boost::asio::io_service& io_service, boost::uint16_t remote_port);
         
      public:

         boost::asio::ip::tcp::socket& socket();

      public:

         void start();

         void send_hello();
         void handle_send_hello(const boost::system::error_code& error);
         void receive_hello_response();
         void handle_receive_hello_response(const boost::system::error_code& error, std::size_t bytes_transferred);
         void receive_device_id();
         void handle_receive_device_id(const boost::system::error_code& error, std::size_t bytes_transferred);
         void send_connect();
         void handle_send_connect(const boost::system::error_code& error);
         void receive_connect_response();
         void handle_receive_connect_response(const boost::system::error_code& error, std::size_t bytes_transferred);

         void read_from_client();
         void handle_read_from_client(const boost::system::error_code& error, std::size_t bytes_transferred);
         void write_to_usbmux();
         void handle_write_to_usbmux(const boost::system::error_code& error, std::size_t bytes_transferred);

         void read_from_usbmux();
         void handle_read_from_usbmux(const boost::system::error_code& error, std::size_t bytes_transferred);
         void write_to_client();
         void handle_write_to_client(const boost::system::error_code& error);
         
      private:
         
         boost::asio::ip::tcp::socket client_socket_;
         boost::asio::local::stream_protocol::socket usbmux_socket_;
         boost::asio::local::stream_protocol::socket usbmux_socket2_;
         boost::asio::local::stream_protocol::endpoint usbmux_endpoint_;
         boost::uint16_t remote_port_;
         
         char buffer_data_[32 * 1024];
         size_t buffer_length_;

         char client_buffer_data_[32 * 1024];
         size_t client_buffer_length_;
         
         boost::uint8_t device_id_;
   };
   
   typedef boost::shared_ptr<proxy_session> proxy_session_ptr;
   
   class proxy
   {
      public:
         
         proxy(boost::asio::io_service& io_service, const boost::asio::ip::tcp::endpoint& endpoint, boost::uint16_t remote_port);

      public:
         
         void handle_accept(proxy_session_ptr session, const boost::system::error_code& error);
         
      private:
         
         boost::asio::io_service& io_service_;
         boost::asio::ip::tcp::acceptor acceptor_;
         boost::uint16_t remote_port_;
   };
      
   typedef boost::shared_ptr<proxy> proxy_ptr;
   typedef std::list<proxy_ptr> proxy_list;
   
}

#endif
