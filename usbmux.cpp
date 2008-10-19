// usbmux.cpp

#include "usbmux.hpp"

namespace usbmux {

   proxy_session::proxy_session(boost::asio::io_service& io_service, boost::uint16_t remote_port)
      : client_socket_(io_service), usbmux_socket_(io_service), usbmux_socket2_(io_service), usbmux_endpoint_("/var/run/usbmuxd"),
        remote_port_(remote_port)
   {
   }
   
   boost::asio::ip::tcp::socket& proxy_session::socket()
   {
      return client_socket_;
   }

   void proxy_session::start()
   {
      boost::system::error_code error = usbmux_socket_.connect(usbmux_endpoint_, error);
      if (!error) {
         send_hello();
      } else {
         std::cerr << "USBMUX Proxy: Could not connect to usbmux" << std::endl;
      }
   }

   //

   void proxy_session::send_hello()
   {
      usbmux_hello_request* r = reinterpret_cast<usbmux_hello_request*>(buffer_data_);
      r->header.length = sizeof(usbmux_hello_request);
      r->header.reserved = 0;
      r->header.type = usbmux_hello;
      r->header.tag = 2;
      
      boost::asio::async_write(
         usbmux_socket_,
         boost::asio::buffer(buffer_data_, sizeof(usbmux_hello_request)),
         boost::bind(&proxy_session::handle_send_hello, shared_from_this(), boost::asio::placeholders::error)
      );            
   }
   
   void proxy_session::handle_send_hello(const boost::system::error_code& error)
   {
      if (!error) {
         receive_hello_response();
      } else {
         std::cerr << "USBMUX Proxy: Could not send hello to usbmux" << std::endl;
      }
   }
   
   void proxy_session::receive_hello_response()
   {
      boost::asio::async_read(
         usbmux_socket_,
         boost::asio::buffer(buffer_data_, sizeof(usbmux_response)),
         boost::asio::transfer_at_least(sizeof(usbmux_response)),
         boost::bind(&proxy_session::handle_receive_hello_response, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
         );            
   }
   
   void proxy_session::handle_receive_hello_response(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error) {
         receive_device_id();
      } else {
         std::cout << "E: Could not receive hello response from usbmux" << std::endl;
      }
   }
   
   void proxy_session::receive_device_id()
   {
      boost::asio::async_read(
         usbmux_socket_,
         boost::asio::buffer(buffer_data_, 0x011c),
         boost::asio::transfer_at_least(0x011c),
         boost::bind(&proxy_session::handle_receive_device_id, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
         );            
   }
   
   void proxy_session::handle_receive_device_id(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error) {
         device_id_ = buffer_data_[16];
         send_connect();
      } else {
         std::cout << "E: Could not receive client id from usbmux" << std::endl;
      }
   }
   
   //
   
   void proxy_session::send_connect()
   {
      boost::system::error_code error = usbmux_socket2_.connect(usbmux_endpoint_, error);
      if (error) {
         std::cout << "Could not connect to usbmux" << std::endl;
         return;
      }
      
      usbmux_connect_request* r = reinterpret_cast<usbmux_connect_request*>(buffer_data_);
      r->header.length = sizeof(usbmux_connect_request);
      r->header.reserved = 0;
      r->header.type = usbmux_connect;
      r->header.tag = 3;
      r->device_id = device_id_;
      r->dst_port = htons(remote_port_);
      r->reserved = 0;
      
      boost::asio::async_write(
         usbmux_socket2_,
         boost::asio::buffer(buffer_data_, sizeof(usbmux_connect_request)),
         boost::bind(&proxy_session::handle_send_connect, shared_from_this(), boost::asio::placeholders::error)
      );            
   }
   
   void proxy_session::handle_send_connect(const boost::system::error_code& error)
   {
      receive_connect_response();
   }
   
   void proxy_session::receive_connect_response()
   {
      boost::asio::async_read(
         usbmux_socket2_,
         boost::asio::buffer(buffer_data_, sizeof(usbmux_response)),
         boost::asio::transfer_at_least(sizeof(usbmux_response)),
         boost::bind(&proxy_session::handle_receive_connect_response, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
      );            
   }
   
   void proxy_session::handle_receive_connect_response(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error) {
         read_from_usbmux();
         read_from_client();
      } else {
         std::cout << "Could not receive connect response usbmux" << std::endl;
      }
   }
   
   //
   
   void proxy_session::read_from_client()
   {
      client_socket_.async_read_some(
         boost::asio::buffer(client_buffer_data_, sizeof(client_buffer_data_)),
         boost::bind(&proxy_session::handle_read_from_client, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
      );
   }
   
   void proxy_session::handle_read_from_client(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error && bytes_transferred > 0) {
         client_buffer_length_ = bytes_transferred;
         write_to_usbmux();
      } else {
         std::cout << "Could not read from client" << std::endl;
      }
   }
   
   void proxy_session::write_to_usbmux()
   {
      boost::asio::async_write(
         usbmux_socket2_,
         boost::asio::buffer(client_buffer_data_, client_buffer_length_),
         boost::bind(&proxy_session::handle_write_to_usbmux, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
      );
   }
   
   void proxy_session::handle_write_to_usbmux(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error) {
         read_from_client();
      } else {
         std::cout << "Could not write to usbmux" << std::endl;
      }
   }         
   
   //
   
   void proxy_session::read_from_usbmux()
   {
      usbmux_socket2_.async_read_some(
         boost::asio::buffer(buffer_data_, sizeof(buffer_data_)),
         boost::bind(&proxy_session::handle_read_from_usbmux, shared_from_this(), boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)
      );
   }
   
   void proxy_session::handle_read_from_usbmux(const boost::system::error_code& error, std::size_t bytes_transferred)
   {
      if (!error && bytes_transferred > 0) {
         buffer_length_ = bytes_transferred;
         write_to_client();
      } else {
         std::cout << "Could not read from usbmux" << std::endl;
      }
   }
   
   void proxy_session::write_to_client()
   {
      boost::asio::async_write(
         client_socket_,
         boost::asio::buffer(buffer_data_, buffer_length_),
         boost::bind(&proxy_session::handle_write_to_client, shared_from_this(), boost::asio::placeholders::error)
      );
   }
   
   void proxy_session::handle_write_to_client(const boost::system::error_code& error)
   {
      if (!error) {
         read_from_usbmux();
      } else {
         std::cerr << "USBMUX PROXY: Could not write to client" << std::endl;
      }
   }
   
   //
         
   proxy::proxy(boost::asio::io_service& io_service, const boost::asio::ip::tcp::endpoint& endpoint, boost::uint16_t remote_port)
      : io_service_(io_service), acceptor_(io_service, endpoint), remote_port_(remote_port)
   {
      proxy_session_ptr new_session(new proxy_session(io_service_, remote_port_));
      acceptor_.async_accept(
         new_session->socket(),
         boost::bind(&proxy::handle_accept, this, new_session, boost::asio::placeholders::error)
      );
      
      std::cout << "Foooo" << std::endl;
   }
   
   void proxy::handle_accept(proxy_session_ptr session, const boost::system::error_code& error)
   {
      if (!error) {
         session->start();
         proxy_session_ptr new_session(new proxy_session(io_service_, remote_port_));
         acceptor_.async_accept(
            new_session->socket(),
            boost::bind(&proxy::handle_accept, this, new_session, boost::asio::placeholders::error)
         );
      } else {
         std::cerr << "USBMUX Proxy: Cannot accept new connection" << std::endl;
      }
   }
   
}

