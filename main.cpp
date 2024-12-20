#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include "kuznyechik.hpp"
#include "block128.hpp"

block128 create_random_block() {
    std::array<uint8_t, 16> block;
    for(int i = 0; i < 16; i++) {
        block[i] = rand() % 256;
    }
    return {block};
}


bool test_string(kuznyechik& kuzya) {
    std::string in = "ffeeddccbbaa99881122334455667700";
    auto bl = block128(in);
    return bl.to_string() == in;
}

bool test_S(kuznyechik& kuzya) {
    std::pair<std::string, std::string> tcs[4] = {
            {"ffeeddccbbaa99881122334455667700", "b66cd8887d38e8d77765aeea0c9a7efc"},
            {"b66cd8887d38e8d77765aeea0c9a7efc", "559d8dd7bd06cbfe7e7b262523280d39"},
            {"559d8dd7bd06cbfe7e7b262523280d39", "0c3322fed531e4630d80ef5c5a81c50b"},
            {"0c3322fed531e4630d80ef5c5a81c50b", "23ae65633f842d29c5df529c13f5acda"}
    };
    for (auto &tc: tcs) {
        auto in = tc.first;
        auto exp = tc.second;
        auto bl = block128(in);
        kuzya.S(bl);
        if (bl.to_string() != exp) {
            return false;
        }
    }
    return true;
}

bool test_R(kuznyechik& kuzya) {
    std::pair<std::string, std::string> tcs[4] = {
            {"00000000000000000000000000000100", "94000000000000000000000000000001"},
            {"94000000000000000000000000000001", "a5940000000000000000000000000000"},
            {"a5940000000000000000000000000000", "64a59400000000000000000000000000"},
            {"64a59400000000000000000000000000", "0d64a594000000000000000000000000"}
    };
    for (auto &tc: tcs) {
        auto in = tc.first;
        auto exp = tc.second;
        auto bl = block128(in);
        kuzya.R(bl);
        if (bl.to_string() != exp) {
            std::cout << bl.to_string() << " " << exp << std::endl;
            return false;
        }
    }
    return true;
}

bool test_L(kuznyechik& kuzya) {
    std::pair<std::string, std::string> tcs[4] = {
            {"64a59400000000000000000000000000", "d456584dd0e3e84cc3166e4b7fa2890d"},
            {"d456584dd0e3e84cc3166e4b7fa2890d", "79d26221b87b584cd42fbc4ffea5de9a"},
            {"79d26221b87b584cd42fbc4ffea5de9a", "0e93691a0cfc60408b7b68f66b513c13"},
            {"0e93691a0cfc60408b7b68f66b513c13", "e6a8094fee0aa204fd97bcb0b44b8580"}
    };
    for (auto &tc: tcs) {
        auto in = tc.first;
        auto exp = tc.second;
        auto bl = block128(in);
        kuzya.L(bl);
        if (bl.to_string() != exp) {
            std::cout << bl.to_string() << " " << exp << '\n';
            return false;
        }
    }
    return true;
}

bool test_LS(kuznyechik& kuzya) {
    std::string tcs[4] = {
            "d456584dd0e3e84cc3166e4b7fa2890d",
            "64a59400000000000000000000000000",
            "79d26221b87b584cd42fbc4ffea5de9a",
            "0e93691a0cfc60408b7b68f66b513c13"
    };

    for (auto &tc: tcs) {
        auto tt = tc;
        auto ls = block128(tc);
        auto sl = block128(tc);
        auto s = block128(tc);
        kuzya.ApplyLS(ls, kuzya.enc_ls_table);

        kuzya.S(s);
        kuzya.L(s);


        if (ls.to_string() != s.to_string()) {
            std::cerr << "test_LS: got: '" << ls.to_string() << "', expected: '" << s.to_string() << "'\n";
            return false;
        }
    }
    return true;
}

bool test_LSX(kuznyechik& kuzya) {
    block128 key = block128("8899aabbccddeeff0011223344556677");
    block128 C = block128("6ea276726c487ab85d27bd10dd849401");
    kuzya.X_k(key, C);
    if (key.to_string() != "e63bdcc9a09594475d369f2399d1f276") {
        return false;
    }
    kuzya.S(key);
    if (key.to_string() != "0998ca37a7947aabb78f4a5ae81b748a") {
        return false;
    }
    kuzya.L(key);
    if (key.to_string() != "3d0940999db75d6a9257071d5e6144a6") {
        return false;
    }
    return true;
}

bool test_F(kuznyechik& kuzya) {
    std::pair<block128, block128> key = {block128("8899aabbccddeeff0011223344556677"),
                                         block128("fedcba98765432100123456789abcdef")};
    block128 C = block128("6ea276726c487ab85d27bd10dd849401");
    kuzya.F_k(C, key);
    if (key.first.to_string() != "c3d5fa01ebe36f7a9374427ad7ca8949" ||
        key.second.to_string() != "8899aabbccddeeff0011223344556677") {
        std::cout << key.first.to_string() << " " << key.second.to_string() << std::endl;
        return false;
    }
    return true;
}

bool test_set_next_key(kuznyechik& kuzya) {
    std::pair<block128, block128> key = {block128("8899aabbccddeeff0011223344556677"),
                                         block128("fedcba98765432100123456789abcdef")};

    std::string transes[19] = {
            "",
            "8899aabbccddeeff0011223344556677",
            "fedcba98765432100123456789abcdef",
            "c3d5fa01ebe36f7a9374427ad7ca8949",
            "8899aabbccddeeff0011223344556677",
            "37777748e56453377d5e262d90903f87",
            "c3d5fa01ebe36f7a9374427ad7ca8949",
            "f9eae5f29b2815e31f11ac5d9c29fb01",
            "37777748e56453377d5e262d90903f87",
            "e980089683d00d4be37dd3434699b98f",
            "f9eae5f29b2815e31f11ac5d9c29fb01",
            "b7bd70acea4460714f4ebe13835cf004",
            "e980089683d00d4be37dd3434699b98f",
            "1a46ea1cf6ccd236467287df93fdf974",
            "b7bd70acea4460714f4ebe13835cf004",
            "3d4553d8e9cfec6815ebadc40a9ffd04",
            "1a46ea1cf6ccd236467287df93fdf974",
            "db31485315694343228d6aef8cc78c44",
            "3d4553d8e9cfec6815ebadc40a9ffd04"
    };

    for (std::size_t j = 1; j < 9; j++) {
        block128 iter_const = kuzya.get_iterative_const(j);
        kuzya.F_k(iter_const, key);
        if (key.first.to_string() != transes[j * 2 + 1]) {
            std::cout << key.first.to_string() << " " << transes[j * 2 + 1] << std::endl;
            return false;
        }
    }


    return true;
}

bool test_set_keys() {
    std::pair<block128, block128> key = {block128("8899aabbccddeeff0011223344556677"),
                                         block128("fedcba98765432100123456789abcdef")};

    std::string keys[11] = {
            "",
            "8899aabbccddeeff0011223344556677",
            "fedcba98765432100123456789abcdef",
            "db31485315694343228d6aef8cc78c44",
            "3d4553d8e9cfec6815ebadc40a9ffd04",
            "57646468c44a5e28d3e59246f429f1ac",
            "bd079435165c6432b532e82834da581b",
            "51e640757e8745de705727265a0098b1",
            "5a7925017b9fdd3ed72a91a22286f984",
            "bb44e25378c73123a5f32f73cdb6e517",
            "72e9dd7416bcf45b755dbaa88e4a4043"
    };

    auto kuzya = kuznyechik(key);
    auto got_keys = kuzya.get_iterative_keys();
    for (size_t i = 1; i < 11; i++) {
        if (got_keys[i].to_string() != keys[i]) {
            std::cout << got_keys[i].to_string() << " " << keys[i] << std::endl;
            if (i == 10) {
                return false;
            }
        }
    }

    return true;
}

bool test_cyphertext() {
    block128 in = block128("1122334455667700ffeeddccbbaa9988");

    kuznyechik kuzya = kuznyechik({block128("8899aabbccddeeff0011223344556677"),
                                   block128("fedcba98765432100123456789abcdef")});

    kuzya.encrypt(in);
    if(in.to_string() != "7f679d90bebc24305a468d42b9d4edcd") {
        return false;
    }
    return true;
}

bool test_decrypt() {
    block128 cypher = block128("7f679d90bebc24305a468d42b9d4edcd");

    kuznyechik kuzya = kuznyechik({block128("8899aabbccddeeff0011223344556677"),
                                   block128("fedcba98765432100123456789abcdef")});

    kuzya.decrypt(cypher);
    if(cypher.to_string() != "1122334455667700ffeeddccbbaa9988") {
        std::cout << cypher.to_string() << " 1122334455667700ffeeddccbbaa9988\n";
        return false;
    }
    return true;
}


void check_test_res(std::string name, bool res) {
    if (!res) {
        std::cerr << name << ": FAILED!" << std::endl;
    } else {
        std::cout << name << ": OK" << std::endl;
    }
}


void correctness_test() {
    std::pair<block128, block128> key = {create_random_block(), create_random_block()};
    auto kuzya = kuznyechik(key);

    check_test_res("Test string", test_string(kuzya));
    check_test_res("Test S", test_S(kuzya));
    check_test_res("Test R", test_R(kuzya));
    check_test_res("Test L", test_L(kuzya));
    check_test_res("Test LS", test_LS(kuzya));
    check_test_res("Test LSX", test_LSX(kuzya));
    check_test_res("Test F", test_F(kuzya));
    check_test_res("Test set next key", test_set_next_key(kuzya));
    check_test_res("Test setting keys", test_set_keys());
    check_test_res("Test cypher a block", test_cyphertext());
    check_test_res("Test decrypt a block", test_decrypt());
}


void performance_test() {
    std::size_t BLOCKS_IN_100Mb = 6250000;

    long long elapsed_total = 0;

    std::pair<block128, block128> key = {create_random_block(), create_random_block()};
    auto kuzya = kuznyechik(key);

    std::vector<block128> data;
    for (std::size_t i = 0; i < BLOCKS_IN_100Mb; i++) {
        data.emplace_back(create_random_block()); // make random block
    }
    std::cout << "START PERFORMANCE TEST" << std::endl;
    for (std::size_t iter = 0; iter < 10; iter++) { // 10Gb of data
        auto start = std::chrono::system_clock::now();
        for (std::size_t i = 0; i < BLOCKS_IN_100Mb; i++) {
            kuzya.encrypt(data[i]);
//            kuzya.decrypt(data[i]);
        }
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << iter << " : " << elapsed.count() / 1000.0 << " sec\n";
        elapsed_total += elapsed.count();
    }

    double seconds_total = (elapsed_total * 1.0) / 1000;
    double seconds_100Mb = seconds_total / 10;
    int speed = ceil(100 / seconds_100Mb);

    std::cout << "ENCRYPTING\n";
    std::cout << "1Gb of data was processed in " << seconds_total << " seconds" << std::endl;
    std::cout << "Average time of processing 100Mb of data is " << seconds_100Mb << " seconds" << std::endl;
    std::cout << "Total speed of algorithm is " << speed << " Mb/sec\n";

    elapsed_total = 0;

    for (std::size_t iter = 0; iter < 10; iter++) { // 10Gb of data
        auto start = std::chrono::system_clock::now();
        for (std::size_t i = 0; i < BLOCKS_IN_100Mb; i++) {
//            kuzya.encrypt(data[i]);
            kuzya.decrypt(data[i]);
        }
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << iter << " : " << elapsed.count() / 1000.0 << " sec\n";
        elapsed_total += elapsed.count();
    }

    seconds_total = (elapsed_total * 1.0) / 1000;
    seconds_100Mb = seconds_total / 10;
    speed = ceil(100 / seconds_100Mb);

    std::cout << "DECRYPTING\n";
    std::cout << "1Gb of data was processed in " << seconds_total << " seconds" << std::endl;
    std::cout << "Average time of processing 100Mb of data is " << seconds_100Mb << " seconds" << std::endl;
    std::cout << "Total speed of algorithm is " << speed << " Mb/sec\n";
}

int main() {
    correctness_test();
    performance_test();
}

















