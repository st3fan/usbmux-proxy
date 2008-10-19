// usbmux-proxy.hpp

#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>

#include "usbmux.hpp"

bool parse_port(std::string const& arg, boost::uint16_t& port, boost::uint16_t& local_port)
{
   if (arg.find(':')) {
      std::string a = arg.substr(0, arg.find(':'));
      std::string b = arg.substr(arg.find(':') + 1);
      port = std::atoi(a.c_str());
      local_port = std::atoi(b.c_str());
   } else {
      port = local_port = std::atoi(arg.c_str());
   }
   
   return (port != 0) && (local_port != 0);
}

int main(int argc, char** argv)
{
   if (argc < 2) {
      std::cerr << "Usage: usbmux-proxy <port>[:local-port] [<port>[:local-port] ...]\n";
      return 1;
   }
   
   try
   {
      // Check port specifications

      for (int i = 1; i < argc; ++i)  {
         boost::uint16_t port, local_port;
         if (!parse_port(argv[i], port, local_port)) {
            std::cerr << "Invalid port specification: " << argv[1] << std::endl;
            return 1;
         }
      }      

      // Start the proxies
      
      boost::asio::io_service io_service;
      usbmux::proxy_list proxies;
      
      for (int i = 1; i < argc; ++i) {
         boost::uint16_t port, local_port;
         if (parse_port(argv[i], port, local_port)) {
            std::cout << "Creating usbmux proxy for port " << local_port << " (local) to " << port << " (iphone)" << std::endl;
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), local_port);
            usbmux::proxy_ptr proxy(new usbmux::proxy(io_service, endpoint, port));
            proxies.push_back(proxy);
         }
      }

      io_service.run();
   }

   catch (std::exception& e)
   {
      std::cerr << "Exception: " << e.what() << "\n";
   }
   
   return 0;
}
