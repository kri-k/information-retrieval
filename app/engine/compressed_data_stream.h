#pragma once

#include "../../codec.h"


template<typename TData>
class CompressedDataStream {
public:
    virtual ~CompressedDataStream() {}
    virtual TData get() = 0;
    virtual void next() = 0;
    virtual bool end() = 0;
    virtual void clear() = 0;
};


template<typename TData>
class VBDataStream : public CompressedDataStream<TData> {
private:
    TData cur;
    bool isEnd;
    VB<TData, int8_t> codec;
    int8_t *data;
public:
    VBDataStream(int8_t *data, unsigned int size) {
        this->data = data;
        codec = VB<TData, int8_t>(data, size);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    }

    TData get() override {
        return cur;
    }

    void next() override {
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    }

    bool end() override {
        return isEnd;
    }

    void clear() override {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
};


template<typename TData>
class VHBDataStream : public CompressedDataStream<TData> {
private:
    TData cur;
    bool isEnd;
    VHB<TData, int8_t> codec;
    int8_t *data;
public:
    VHBDataStream(int8_t *data, unsigned int size) {
        this->data = data;
        codec = VHB<TData, int8_t>(data, size);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    }

    TData get() override {
        return cur;
    }

    void next() override {
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    }

    bool end() override {
        return isEnd;
    }

    void clear() override {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
};
