#include "../src/slic3r/GUI/ZHCostModel.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
int main()
{
    using namespace ZHStudio;
    const auto cost = estimate_cost({100, 2, 25, 150, 0.35, 0.10});
    require(std::abs(cost.material - 2.5) < 1e-10, "kg conversion");
    require(std::abs(cost.kwh - 0.3) < 1e-10, "W to kWh conversion");
    require(std::abs(cost.energy - 0.105) < 1e-10, "electricity cost");
    require(std::abs(cost.wear - 0.2) < 1e-10, "wear cost");
    require(std::abs(cost.total - 2.805) < 1e-10, "total");
    require(estimate_cost({0,0,0,0,0,0}).total == 0, "zero inputs");
    for (double invalid : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        for (int field = 0; field < 6; ++field) {
            CostInput in;
            switch(field) {
            case 0: in.grams=invalid; break; case 1: in.hours=invalid; break;
            case 2: in.filament_per_kg=invalid; break; case 3: in.watts=invalid; break;
            case 4: in.electricity_per_kwh=invalid; break; case 5: in.wear_per_hour=invalid; break;
            }
            bool rejected=false;
            try { estimate_cost(in); } catch (const std::invalid_argument&) { rejected=true; }
            require(rejected,"invalid input must be rejected for every field");
        }
    }
    bool overflow=false;
    try { estimate_cost({std::numeric_limits<double>::max(),2,std::numeric_limits<double>::max(),150,0.35,0.10}); }
    catch(const std::overflow_error&) { overflow=true; }
    require(overflow,"overflow is rejected");
    int parser_checks = 0;
    for (const char* value : {"", " ", "NaN", "nan", "inf", "Infinity", "1e3", "-1", "+1", "12 EUR", "1 000", "1,000.00", "1.000,00", "1..2", "1,2,3", ".", ",", "2.", "2,", "1001"}) {
        require(!parse_decimal(value,1000).has_value(), "invalid text rejected"); ++parser_checks;
    }
    for (const char* value : {"0.35", "0,35", " 0,35 ", ".35", ",35"}) {
        const auto parsed=parse_decimal(value,1000);
        require(parsed && std::abs(*parsed-0.35)<1e-10,"decimal comma/point accepted"); ++parser_checks;
    }
    require(parse_decimal("1000",1000).value()==1000,"upper bound accepted"); ++parser_checks;
    require(!parse_decimal(std::string(65,'0'),1000),"oversized input rejected"); ++parser_checks;
    require(parse_decimal("0",0).value()==0,"zero boundary accepted"); ++parser_checks;
    std::cout << "ZH cost model: 25 arithmetic checks and " << parser_checks << " input validation checks passed.\n";
    return 0;
}
