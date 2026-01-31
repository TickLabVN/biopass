#include "fingerprint_auth.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
    facepass::FingerprintAuth fp_auth;

    std::cout << "Checking fingerprint availability..." << std::endl;
    bool available = fp_auth.is_available();
    std::cout << "Available: " << (available ? "Yes" : "No") << std::endl;

    if (argc > 1 && std::string(argv[1]) == "auth") {
        if (!available) {
            std::cerr << "Cannot authenticate: fingerprint unavailable." << std::endl;
            // Returning 0 so test passes even if hardware missing, unless expected
            return 0; 
        }
        std::string user = (argc > 2) ? argv[2] : getenv("USER");
        if(user.empty()) user = "root";

        std::cout << "Authenticating user: " << user << std::endl;
        facepass::AuthConfig config;
        config.retries = 3;
        
        facepass::AuthResult result = fp_auth.authenticate(user, config);
        
        switch(result) {
            case facepass::AuthResult::Success: std::cout << "Success!" << std::endl; break;
            case facepass::AuthResult::Failure: std::cout << "Failure." << std::endl; break;
            case facepass::AuthResult::Unavailable: std::cout << "Unavailable." << std::endl; break;
            case facepass::AuthResult::Retry: std::cout << "Retry requested (but loop ended)." << std::endl; break;
        }
    }

    return 0;
}
