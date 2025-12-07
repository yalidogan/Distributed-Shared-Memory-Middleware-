#pragma once 

#include <string>
#include <memory>
#include <vector>


#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/support/server_interceptor.h>
#include <grpcpp/support/interceptor.h>

#include "interceptors/logger.hpp"


class Server 
{
public: 
    explicit Server(std::string server_address, std::shared_ptr<grpc::Service> service = nullptr, std::string service_name = "Server"); 

    void Start(); //start serving reqs 
    void Stop(); //Stop serving 

private:
    std::unique_ptr<grpc::Server> server_;
    std::shared_ptr<grpc::Service> service_;
    std::string server_address_; 
    std::string service_name_; 

    //Interceptors (Middleware) 
    std::vector<std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>> interceptor_creators_; 

};

