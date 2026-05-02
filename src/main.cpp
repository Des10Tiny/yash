#include "core/yash.hpp"
#include "utils/yash_error.hpp"

#include <iostream>

int main() {
    try {
        Yash shell;
        return shell.Run();

    } catch (const YashExitException& e) {
        return e.GetCode();

    } catch (const std::exception& e) {
        std::cerr << "yash: fatal boot error: " << e.what() << '\n';
        return 1;

    } catch (...) {
        std::cerr << "yash: unknown fatal error\n";
        return 2;
    }
}