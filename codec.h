#pragma once

#include <vector>


/* Variable byte */
template<typename TData, typename TBlock>
class VB {
private:
	static const int blockSize = sizeof(TBlock) * 8;
	static const TBlock blockMask = (1 << (blockSize - 1)) - 1;
	static const TBlock endMask = 1 << (blockSize - 1);

	TBlock *encodedData;
	unsigned int encodedDataSize;

	TData num;
	int offset;
	unsigned int id;
public:
	VB() {
		encodedData = nullptr;
		encodedDataSize = 0;
		reset();
	}

	VB(TBlock *encodedData, unsigned int size) {
		this->encodedData = encodedData;
		encodedDataSize = size;
		reset();
	}

	void reset() {
		num = 0;
		offset = 0;
		id = 0;
	}

	TData decodeNext() {
		while (id < encodedDataSize) {
			TBlock i = encodedData[id++];
			num |= (TData)(i & blockMask) << ((blockSize - 1) * offset);
			++offset;
			if (i & endMask) {
				TData tmp = num;
				num = 0;
				offset = 0;
				return tmp;
			}
		}
		return 0;
	}

	bool end() {
		return id >= encodedDataSize;
	}

	unsigned int getOffset() {
		return id * blockSize;
	}

	void setOffset(unsigned int offset) {
		id = offset / blockSize;
	}

	static unsigned int encode(const std::vector<TData> &data, std::vector<TBlock> &encodedData) {
		TBlock curBlock;

		for (TData num : data) {
			do {
				curBlock = num & blockMask;
				num >>= blockSize - 1;
				if (num == 0) {
					curBlock |= endMask;
				}
				encodedData.push_back(curBlock);
			} while (num);
		}

		return encodedData.size() * blockSize;
	}

	static unsigned int bitSize(TData n) {
		unsigned int sz = 0;
		do {
			sz += blockSize;
			n >>= blockSize - 1;
		} while (n);
		return sz;
	}
};


/* Variable half-byte */
template<typename TData, typename TBlock>
class VHB {
private:
	static const int halfBlockSize = sizeof(TBlock) * 4;
	static const TBlock halfBlockMask = (1 << (halfBlockSize - 1)) - 1;
	static const TBlock endMask = 1 << (halfBlockSize - 1);

	TBlock *encodedData;
	unsigned int encodedDataSize;

	TData num;
	int offset;
	int halfNum;
	unsigned int id;
public:
	VHB() {
		encodedData = nullptr;
		encodedDataSize = 0;
		reset();
	}

	VHB(TBlock *encodedData, unsigned int size) {
		this->encodedData = encodedData;
		encodedDataSize = size;
		reset();
	}

	void reset() {
		num = 0;
		offset = 0;
		halfNum = 0;
		id = 0;
	}

	bool end() {
		return id >= encodedDataSize || (id == encodedDataSize - 1 && halfNum == 1 && !(encodedData[id] & endMask));
	}

	unsigned int getOffset() {
		return halfBlockSize * (id * 2 + halfNum);
	}

	void setOffset(unsigned int offset) {
		id = offset / (2 * halfBlockSize);
		halfNum = offset - id * halfBlockSize * 2 == 0 ? 0 : 1;
	}

	TData decodeNext() {
		TBlock curHalfBlock;
		while (id < encodedDataSize) {
			TBlock i = encodedData[id];
			if (halfNum == 0) {
				curHalfBlock = (i & (halfBlockMask << halfBlockSize)) >> halfBlockSize;
			} else {
				curHalfBlock = i & halfBlockMask;
			}

			num |= (TData)curHalfBlock << ((halfBlockSize - 1) * offset);
			++offset;

			if (halfNum == 0) {
				halfNum = 1;
				if (i & (endMask << halfBlockSize)) {
					TData tmp = num;
					num = 0;
					offset = 0;
					return tmp;
				}
			} else {
				id++;
				halfNum = 0;
				if (i & endMask) {
					TData tmp = num;
					num = 0;
					offset = 0;
					return tmp;
				}
			}
		}

		return 0;
	}

	static unsigned int encode(const std::vector<TData> &data, std::vector<TBlock> &encodedData) {
		TBlock curBlock = 0;
		TBlock curHalfBlock;
		int halfNum = 0;

		for (TData num : data) {
			do {
				curHalfBlock = num & halfBlockMask;
				num >>= halfBlockSize - 1;
				if (num == 0) {
					curHalfBlock |= endMask;
				}

				if (halfNum == 0) {
					halfNum = 1;
					curBlock = curHalfBlock << halfBlockSize;
				} else {
					halfNum = 0;
					encodedData.push_back(curBlock | curHalfBlock);
					curBlock = 0;
				}
			} while (num);
		}

		if (halfNum == 1) {
			encodedData.push_back(curBlock);
		}

		return encodedData.size() * halfBlockSize * 2;
	}

	static unsigned int bitSize(TData n) {
		unsigned int sz = 0;
		do {
			sz += halfBlockSize;
			n >>= halfBlockSize - 1;
		} while (n);
		return sz;
	}
};
