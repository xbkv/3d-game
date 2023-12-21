#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

// v, vt, vnのデータを保持するクラス
class Data {
public:
    std::vector<std::vector<float>> v;
    std::vector<std::vector<float>> vt;
    std::vector<std::vector<float>> vn;
};

// 行からv, vt, vnのデータを抽出する関数
std::vector<float> extractValues(std::string line) {
    std::vector<float> values;
    std::istringstream iss(line);
    char type;
    iss >> type; // 'v', 'vt', 'vn'の文字を読み取る

    float value;
    while (iss >> value) {
        values.push_back(value);
    }
    return values;
}

// 値を整形して出力する関数
std::string formatValue(float val) {
    std::ostringstream oss;
    oss << std::showpos << std::fixed << std::setprecision(1) << val << "f";
    return oss.str();
}

int main() {
    std::ifstream file("C:/stage.obj"); // ファイルパスを指定してください
    std::string line;
    Data result;
    std::vector<std::string> vIndices, vtIndices, vnIndices;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.find("v ") == 0) {
                result.v.push_back(extractValues(line));
            } else if (line.find("vt ") == 0) {
                result.vt.push_back(extractValues(line));
            } else if (line.find("vn ") == 0) {
                result.vn.push_back(extractValues(line));
            } else if (line.find("f ") == 0) {
                std::istringstream iss(line);
                std::string token;
                iss >> token;
                while (iss >> token) {
                    std::istringstream index(token);
                    std::string vIndex, vtIndex, vnIndex;
                    getline(index, vIndex, '/');
                    getline(index, vtIndex, '/');
                    getline(index, vnIndex, '/');
                    vIndices.push_back(vIndex);
                    vtIndices.push_back(vtIndex);
                    vnIndices.push_back(vnIndex);
                }
            }
        }
        file.close();
    } else {
        std::cout << "Unable to open file";
        return 1;
    }

    // 結果を出力
    for (size_t i = 0; i < vIndices.size(); i += 3) {
        std::cout << "{ {";
        for (size_t j = 0; j < 3; ++j) {
            int vIdx = std::stoi(vIndices[i + j]);
            int vtIdx = std::stoi(vtIndices[i + j]);
            int vnIdx = std::stoi(vnIndices[i + j]);
            std::cout << formatValue(result.v[vIdx - 1][0]) << ", " << formatValue(result.v[vIdx - 1][1]) << ", " << formatValue(result.v[vIdx - 1][2]) << "}, {";
            std::cout << formatValue(result.vt[vtIdx - 1][0]) << ", " << formatValue(result.vt[vtIdx - 1][1]) << "}, {";
            std::cout << formatValue(result.vn[vnIdx - 1][0]) << ", " << formatValue(result.vn[vnIdx - 1][1]) << ", " << formatValue(result.vn[vnIdx - 1][2]);
            if (j < 2) {
                std::cout << "} }, ";
            } else {
                std::cout << "} }," << std::endl;
            }
        }
        if (i == 2) {
            std::cout << "// Second triangle" << std::endl;
        }
    }

    return 0;
}
