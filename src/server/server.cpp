#include "server/server.hpp"
#include <iostream>
#include <grpcpp/grpcpp.h>


// If you deleted this file, comment out the lines marked [INTERCEPTOR].
#include "interceptors/logger.hpp" 

Server::Server(std::string server_address, std::shared_ptr<grpc::Service> service, std::string service_name) 
    : server_address_(server_address), service_(service), service_name_(service_name)
{
    //INTERCEPTOR
    //Add the Logger Interceptor factory
    this->interceptor_creators_.emplace_back(std::make_unique<LoggerInterceptorFactory>());
}

void Server::Start()
{
    grpc::ServerBuilder builder; 

    //Configure the Port
    builder.AddListeningPort(this->server_address_, grpc::InsecureServerCredentials()); 

    //Register Interceptors
    //MUST USE STD::MOVE WITH SPECIAL POINTERS 
    //WE CANNOT COPY THESE ONES
    //Transfers the ownership  
    if (!this->interceptor_creators_.empty()) {
        builder.experimental().SetInterceptorCreators(std::move(this->interceptor_creators_));
    }

    //Register the Service Logic
    // We extract the raw pointer from the shared_ptr to give to grpc
    builder.RegisterService(this->service_.get());

    //Build and Start
    this->server_ = builder.BuildAndStart();

    if (this->server_) {
        std::cout << "[ServerWrapper] " << this->service_name_ << " listening on " << this->server_address_ << std::endl;
        
        //Wait Loop 
        //Blocks this thread forever until Shutdown is called
        this->server_->Wait();
    } else {
        std::cerr << "[ServerWrapper] Failed to start server on " << this->server_address_ << std::endl;
    }
}

void Server::Stop()
{
    std::cout << "[ServerWrapper] " << this->service_name_ << " shutting down..." << std::endl;
    if (this->server_) {
        this->server_->Shutdown(); 
    }
    std::cout << "[ServerWrapper] " << this->service_name_ << " stopped." << std::endl;
}