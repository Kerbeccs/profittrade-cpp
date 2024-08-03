#include <cpprest/http_client.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace web;
using namespace web::http;
using namespace web::http::client;
using json = nlohmann::json;

class DepthManager {
public:
    DepthManager(const std::string& market) : market_(market) {
        fetchMarketData();
    }

    void fetchMarketData() {
        // Example URL, adjust as necessary
        http_client client(U("https://api.exchange.com/" + market_ + "/depth"));
        uri_builder builder(U("/"));

        client.request(methods::GET, builder.to_string()).then([this](http_response response) {
            if (response.status_code() == status_codes::OK) {
                response.extract_json().then([this](json::value jsonValue) {
                    marketData_ = jsonValue;
                }).wait();
            }
        }).wait();
    }

    json getRelevantDepth() const {
        return marketData_;
    }

private:
    std::string market_;
    json marketData_;
};

void performArbitrage(const DepthManager& solInrMarket, const DepthManager& usdtInrMarket, const DepthManager& solUsdtMarket) {
    double sol = 1.0;
    double canGetInr = solInrMarket.getRelevantDepth()["lowestAsk"].get<double>() - 0.001;
    double canGetUsdt = canGetInr / usdtInrMarket.getRelevantDepth()["lowestAsk"].get<double>();
    double canGetSol = canGetUsdt / solUsdtMarket.getRelevantDepth()["lowestAsk"].get<double>();

    std::cout << "You can convert " << sol << " SOL into " << canGetSol << " SOL" << std::endl;

    double initialInr = solInrMarket.getRelevantDepth()["highestBid"].get<double>() + 0.001;
    double canGetUsdt2 = solUsdtMarket.getRelevantDepth()["highestBid"].get<double>();
    double canGetInr2 = canGetUsdt2 * usdtInrMarket.getRelevantDepth()["highestBid"].get<double>();

    std::cout << "You can convert " << initialInr << " INR into " << canGetInr2 << " INR" << std::endl;
}

void mainLoop(DepthManager& solInrMarket, DepthManager& usdtInrMarket, DepthManager& solUsdtMarket) {
    while (true) {
        solInrMarket.fetchMarketData();
        usdtInrMarket.fetchMarketData();
        solUsdtMarket.fetchMarketData();

        performArbitrage(solInrMarket, usdtInrMarket, solUsdtMarket);

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    DepthManager solInrMarket("B-XAI_INR");
    DepthManager usdtInrMarket("B-USDT_INR");
    DepthManager solUsdtMarket("B-XAI_USDT");

    std::thread mainThread(mainLoop, std::ref(solInrMarket), std::ref(usdtInrMarket), std::ref(solUsdtMarket));
    mainThread.join();

    return 0;
}
