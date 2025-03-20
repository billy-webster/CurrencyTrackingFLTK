#include <FL/Fl.H>
#include <ctime>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "matplotlibcpp.h" 
#include <iomanip> 
#include <sstream> 

namespace plt = matplotlibcpp; 

using json = nlohmann::json;


double get_conversion_rate(const std::string& from_currency, const std::string& to_currency, const std::string& api_key);


std::vector<double> get_historical_rates(const std::string& currency, const std::string& api_key, int days);


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


double get_conversion_rate(const std::string& from_currency, const std::string& to_currency, const std::string& api_key) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string url = "https://api.currencyapi.com/v3/latest?apikey=" + api_key + "&base_currency=" + from_currency;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return -1;
        } else {
            json j = json::parse(readBuffer);
            if (j.contains("data") && j["data"].contains(to_currency)) {
                double conversion_rate = j["data"][to_currency]["value"].get<double>();
                curl_easy_cleanup(curl);
                return conversion_rate;
            } else {
                std::cerr << "No data found for currency: " << to_currency << std::endl;
                curl_easy_cleanup(curl);
                return -1;
            }
        }
    }
    curl_global_cleanup();
    return -1;
}

std::vector<double> get_historical_rates(const std::string& currency, const std::string& api_key, int days) {
    CURL* curl;
    CURLcode res;
    std::vector<double> rates;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        
        time_t now = time(0);
        tm* ltm = localtime(&now);

       
        for (int i = 0; i < days; ++i) {
            
            time_t current_date = now - (i * 86400); 
            tm* date = localtime(&current_date);

           
            char date_str[11];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", date);

            
            std::string url = "https://api.currencyapi.com/v3/historical?apikey=" + api_key + "&base_currency=" + currency + "&date=" + date_str;

            
            std::string readBuffer;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                continue; 
            }

            
            json j = json::parse(readBuffer);
            if (j.contains("data") && j["data"].contains("USD")) {
                double rate = j["data"]["USD"]["value"].get<double>();
                rates.push_back(rate);
            } else {
                std::cerr << "No data found for date: " << date_str << std::endl;
            }

            
            sleep(1); 
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return rates;
}


class CurrencyTracker {
public:
    CurrencyTracker(int width, int height, const char* title);
    ~CurrencyTracker();

    void run();

private:
    Fl_Window* window;
    Fl_Menu_Bar* menu_bar;
    Fl_Box* home_message;
    Fl_Box* currency_tracking_message;
    Fl_Box* currency_charts_message;
    Fl_Box* current_time;
    Fl_Input* amount_input;
    Fl_Choice* from_currency_choice;
    Fl_Choice* to_currency_choice;
    Fl_Output* result_output;
    Fl_Button* convert_button;
    Fl_Choice* chart_currency_choice;

    static void exit_cb(Fl_Widget*, void*);
    static void home_cb(Fl_Widget*, void*);
    static void currency_tracking_cb(Fl_Widget*, void*);
    static void currency_charts_cb(Fl_Widget*, void*);
    static void convert_cb(Fl_Widget*, void*);
    static void update_current_time(void*);
    static void update_chart_cb(Fl_Widget*, void*);
    void display_home_page();
    void display_currency_tracking_page();
    void display_currency_charts_page();
    void perform_conversion();
    void update_chart();
};


CurrencyTracker::CurrencyTracker(int width, int height, const char* title) {
    window = new Fl_Window(width, height, title);
    menu_bar = new Fl_Menu_Bar(0, 0, width, 30);
    menu_bar->add("Home", FL_CTRL + 'h', home_cb, (void*)this);
    menu_bar->add("Currency Tracking", FL_CTRL + 't', currency_tracking_cb, (void*)this);
    menu_bar->add("Currency Charts", FL_CTRL + 'c', currency_charts_cb, (void*)this);
    menu_bar->add("Exit", FL_CTRL + 'q', exit_cb, (void*)this);

    home_message = new Fl_Box(100, 100, width - 200, 30, "Welcome to the Currency Transfer Application");
    home_message->box(FL_FLAT_BOX);
    home_message->labelsize(18);

    currency_tracking_message = new Fl_Box(100, 150, width - 200, 30, "Convert Currency");
    currency_tracking_message->box(FL_FLAT_BOX);
    currency_tracking_message->labelsize(18);
    currency_tracking_message->hide();

    currency_charts_message = new Fl_Box(100, 150, width - 200, 30, "Currency Charts");
    currency_charts_message->box(FL_FLAT_BOX);
    currency_charts_message->labelsize(18);
    currency_charts_message->hide();

    current_time = new Fl_Box(100, 100, width - 200, 30, "");
    current_time->box(FL_FLAT_BOX);
    current_time->labelsize(18);

    amount_input = new Fl_Input(150, 250, 200, 30, "Amount:");
    from_currency_choice = new Fl_Choice(150, 300, 200, 30, "From Currency:");
    to_currency_choice = new Fl_Choice(150, 350, 200, 30, "To Currency:");
    result_output = new Fl_Output(150, 400, 200, 30, "Result:");

    from_currency_choice->add("USD|EUR|GBP|JPY|AUD|CAD|CHF|CNY|SEK|NZD|MXN|SGD|HKD|NOK|KRW|TRY|INR|RUB|BRL|ZAR|DKK|PLN|TWD|THB|MYR|IDR|HUF|CZK|ILS|PHP|AED|SAR|QAR|KWD|BHD|OMR|JOD|LBP|EGP|ARS|CLP|COP|PEN|VEF|UAH|KZT|UZS|AZN|BYN|GEL|AMD|KGS|TJS|TMT|MNT|AFN|PKR|BDT|LKR|NPR|MMK|KHR|LAK|MOP|MVR|BND|BWP|ISK|XOF|XAF|XCD|XPF|XDR|SDR|ANG|AWG|BBD|BMD|BZD|CUC|CUP|FKP|GIP|GYD|HTG|JMD|KYD|LRD|MUR|NAD|SBD|SCR|SHP|SRD|SSP|TVD|VES|VUV|WST|YER|ZMW|ZWL");
    to_currency_choice->add("USD|EUR|GBP|JPY|AUD|CAD|CHF|CNY|SEK|NZD|MXN|SGD|HKD|NOK|KRW|TRY|INR|RUB|BRL|ZAR|DKK|PLN|TWD|THB|MYR|IDR|HUF|CZK|ILS|PHP|AED|SAR|QAR|KWD|BHD|OMR|JOD|LBP|EGP|ARS|CLP|COP|PEN|VEF|UAH|KZT|UZS|AZN|BYN|GEL|AMD|KGS|TJS|TMT|MNT|AFN|PKR|BDT|LKR|NPR|MMK|KHR|LAK|MOP|MVR|BND|BWP|ISK|XOF|XAF|XCD|XPF|XDR|SDR|ANG|AWG|BBD|BMD|BZD|CUC|CUP|FKP|GIP|GYD|HTG|JMD|KYD|LRD|MUR|NAD|SBD|SCR|SHP|SRD|SSP|TVD|VES|VUV|WST|YER|ZMW|ZWL");

    convert_button = new Fl_Button(150, 450, 200, 30, "Convert");
    convert_button->callback(convert_cb, (void*)this);

    chart_currency_choice = new Fl_Choice(150, 250, 200, 30, "Select Currency:");
    chart_currency_choice->add("USD|EUR|GBP|JPY|AUD|CAD|CHF|CNY|SEK|NZD|MXN|SGD|HKD|NOK|KRW|TRY|INR|RUB|BRL|ZAR|DKK|PLN|TWD|THB|MYR|IDR|HUF|CZK|ILS|PHP|AED|SAR|QAR|KWD|BHD|OMR|JOD|LBP|EGP|ARS|CLP|COP|PEN|VEF|UAH|KZT|UZS|AZN|BYN|GEL|AMD|KGS|TJS|TMT|MNT|AFN|PKR|BDT|LKR|NPR|MMK|KHR|LAK|MOP|MVR|BND|BWP|ISK|XOF|XAF|XCD|XPF|XDR|SDR|ANG|AWG|BBD|BMD|BZD|CUC|CUP|FKP|GIP|GYD|HTG|JMD|KYD|LRD|MUR|NAD|SBD|SCR|SHP|SRD|SSP|TVD|VES|VUV|WST|YER|ZMW|ZWL");
    chart_currency_choice->callback(update_chart_cb, (void*)this);

    display_home_page();
    window->end();

    Fl::add_timeout(1.0, (Fl_Timeout_Handler)update_current_time, (void*)this);
}


CurrencyTracker::~CurrencyTracker() {
    delete window;
}


void CurrencyTracker::run() {
    window->show();
    Fl::run();
}

void CurrencyTracker::display_home_page() {
    home_message->show();
    currency_tracking_message->hide();
    currency_charts_message->hide();
    amount_input->hide();
    from_currency_choice->hide();
    to_currency_choice->hide();
    result_output->hide();
    convert_button->hide();
    chart_currency_choice->hide();
    current_time->hide();
}


void CurrencyTracker::display_currency_tracking_page() {
    home_message->hide();
    currency_tracking_message->show();
    amount_input->show();
    from_currency_choice->show();
    to_currency_choice->show();
    result_output->show();
    convert_button->show();
    chart_currency_choice->hide();
    current_time->show();
}


void CurrencyTracker::display_currency_charts_page() {
    home_message->hide();
    currency_tracking_message->hide();
    amount_input->hide();
    from_currency_choice->hide();
    to_currency_choice->hide();
    result_output->hide();
    convert_button->hide();
    chart_currency_choice->show();
    current_time->show();
}


void CurrencyTracker::perform_conversion() {
    const char* amount_str = amount_input->value();
    double amount = atof(amount_str);

    std::string from_currency = from_currency_choice->text(from_currency_choice->value());
    std::string to_currency = to_currency_choice->text(to_currency_choice->value());

    double conversion_rate = get_conversion_rate(from_currency, to_currency, "INSERT API KEY");
    
    if (conversion_rate != -1) {
        double converted_amount = amount * conversion_rate;
        result_output->value(std::to_string(converted_amount).c_str());
    } else {
        result_output->value("Error");
    }
}


void CurrencyTracker::update_chart() {
    std::string selected_currency = chart_currency_choice->text(chart_currency_choice->value());

    
    std::vector<double> rates = get_historical_rates(selected_currency, "cur_live_Cjy4kti6CtdP5sNRaFG5rSn3BosWzhDcaEI4xOrR", 30); 

    if (rates.empty()) {
        std::cerr << "No historical data found for " << selected_currency << std::endl;
        return;
    }

    
    std::vector<double> days(rates.size());
    for (size_t i = 0; i < days.size(); ++i) {
        days[i] = i + 1;
    }

    
    plt::plot(days, rates);
    plt::title("Historical Exchange Rates for " + selected_currency);
    plt::xlabel("Days");
    plt::ylabel("Exchange Rate");
    plt::show();
}


void CurrencyTracker::update_current_time(void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;

    time_t now = time(0);
    tm *ltm = localtime(&now);

    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", ltm);

    app->current_time->copy_label(time_str);

    Fl::add_timeout(1.0, (Fl_Timeout_Handler)update_current_time, (void*)app);
}


void CurrencyTracker::exit_cb(Fl_Widget*, void*) {
    std::cout << "Exiting application..." << std::endl;
    exit(0);
}


void CurrencyTracker::home_cb(Fl_Widget*, void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;
    app->display_home_page();
}


void CurrencyTracker::currency_tracking_cb(Fl_Widget*, void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;
    app->display_currency_tracking_page();
}


void CurrencyTracker::currency_charts_cb(Fl_Widget*, void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;
    app->display_currency_charts_page();
}


void CurrencyTracker::convert_cb(Fl_Widget*, void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;
    app->perform_conversion();
}


void CurrencyTracker::update_chart_cb(Fl_Widget*, void* data) {
    CurrencyTracker* app = (CurrencyTracker*)data;
    app->update_chart();
}


int main() {
    CurrencyTracker app(500, 500, "Currency Tracker");
    app.run();
    return 0;
}
