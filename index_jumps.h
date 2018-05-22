#pragma once

#include <vector>
#include <cmath>
#include <algorithm>


namespace Jump {
    inline unsigned int min(unsigned int a, unsigned int b) {
        return a < b ? a : b;
    }

    inline unsigned int clamp(unsigned int n) {
        return n > 1 ? n : 1;
    }

    bool isJump(unsigned int curPos, unsigned int jumpLength, unsigned int totalLength) {
        if (jumpLength <= 10 || (totalLength - curPos) <= 10) return false;
        return curPos % jumpLength == 0;
    }

    unsigned int jumpLength(unsigned int totalLength) {
        return min(clamp(sqrt(totalLength)), 100);
    }

    template<typename TCodec, typename TData>
    void insertJumps(const std::vector<TData> &data, std::vector<TData> &result) {
        std::vector<unsigned int> suffDataSum = {0};
        std::vector<unsigned int> suffOffsetSum = {0};

        unsigned int jlen = jumpLength(data.size());

        for (int i = data.size() - 1; i >= 0; --i) {
            if (isJump(i, jlen, data.size())) {
                int idJumpEnd = std::max<int>(1, suffDataSum.size() - (int)jlen);
                unsigned int addSum = suffDataSum.back() - suffDataSum[idJumpEnd];
                unsigned int offsetSum = suffOffsetSum.back() - suffOffsetSum[idJumpEnd];
                result.push_back(offsetSum);
                result.push_back(addSum);
                suffOffsetSum.push_back(
                    suffOffsetSum.back() + 
                    TCodec::bitSize(data[i]) + 
                    TCodec::bitSize(addSum) + 
                    TCodec::bitSize(offsetSum));
            } else {
                suffOffsetSum.push_back(suffOffsetSum.back() + TCodec::bitSize(data[i]));
            }
            result.push_back(data[i]);
            suffDataSum.push_back(suffDataSum.back() + data[i]);
        }
        std::reverse(result.begin(), result.end());
    }
};
