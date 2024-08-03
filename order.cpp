#include <cpprest/http_client.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <openssl/hmac.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::client;
using json = nlohmann::json;

const std::string baseurl = "https://api.coindcx.com";
const std::string key = "YOUR_API_KEY";
const std::string secret = "YOUR_API_SECRET";

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), NULL, NULL);
    char mdString[65];
    for(int i = 0; i < 32; i++) {
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    return std::string(mdString);
}

void createOrder(const std::string& side, const std::string& market, double price, double quantity, const std::string& clientOrderId) {
    json body = {
        {"side", side},
        {"order_type", "limit_order"},
        {"market", market},
        {"price_per_unit", price},
        {"total_quantity", quantity},
        {"timestamp", get_current_timestamp()},
        {"client_order_id", clientOrderId}
    };

    std::string payload = body.dump();
    std::string signature = hmac_sha256(secret, payload);

    http_client client(U(baseurl + "/exchange/v1/orders/create"));
    http_request request(methods::POST);
    request.headers().add(U("X-AUTH-APIKEY"), U(key));
    request.headers().add(U("X-AUTH-SIGNATURE"), U(signature));
    request.headers().set_content_type(U("application/json"));
    request.set_body(payload);

    client.request(request).then([](http_response response) {
        if (response.status_code() == status_codes::OK) {
            std::cout << "Order created successfully" << std::endl;
            std::cout << response.extract_json().get() << std::endl;
        } else {
            std::cout << "Error creating order" << std::endl;
        }
    }).wait();
}

void cancelAll(const std::string& market) {
    json body = {
        {"market", market},
        {"timestamp", get_current_timestamp()}
    };

    std::string payload = body.dump();
    std::string signature = hmac_sha256(secret, payload);

    http_client client(U(baseurl + "/exchange/v1/orders/cancel_all"));
    http_request request(methods::POST);
    request.headers().add(U("X-AUTH-APIKEY"), U(key));
    request.headers().add(U("X-AUTH-SIGNATURE"), U(signature));
    request.headers().set_content_type(U("application/json"));
    request.set_body(payload);

    client.request(request).then([](http_response response) {
        if (response.status_code() == status_codes::OK) {
            std::cout << "Cancelled all orders successfully" << std::endl;
            std::cout << response.extract_json().get() << std::endl;
        } else {
            std::cout << "Error cancelling orders" << std::endl;
        }
    }).wait();
}

int main() {
    std::string side = "buy";
    std::string market = "XAIINR";
    double price = 100.0;
    double quantity = 10.0;
    std::string clientOrderId = "order123";

    createOrder(side, market, price, quantity, clientOrderId);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    cancelAll(market);

    return 0;
}
