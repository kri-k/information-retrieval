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
    virtual CompressedDataStream<TData>* copy() = 0;
    virtual unsigned int getOffset() = 0;
    virtual void setOffset(unsigned int) = 0;
};


template<typename TData>
class VBDataStream : public CompressedDataStream<TData> {
private:
    TData cur;
    bool isEnd;
    VB<TData, int8_t> codec;
    int8_t *data;
    unsigned int offset;
public:
    VBDataStream(int8_t *data, unsigned int size) {
        this->data = data;
        codec = VB<TData, int8_t>(data, size);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
        offset = 0;
    }

    VBDataStream(const VBDataStream &other) {
        data = other.data;
        codec = other.codec;
        codec.reset();
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
        offset = 0;
    }

    TData get() override {
        return cur;
    }

    void next() override {
        offset = codec.getOffset();
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

    CompressedDataStream<TData>* copy() override {
        return new VBDataStream<TData>(*this);
    }

    unsigned int getOffset() override {
        return offset;
    };

    void setOffset(unsigned int offset) override {
        this->offset = offset;
        codec.setOffset(offset);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    };
};


template<typename TData>
class VHBDataStream : public CompressedDataStream<TData> {
private:
    TData cur;
    bool isEnd;
    VHB<TData, int8_t> codec;
    int8_t *data;
    unsigned int offset;
public:
    VHBDataStream(int8_t *data, unsigned int size) {
        this->data = data;
        codec = VHB<TData, int8_t>(data, size);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
        offset = 0;
    }

    VHBDataStream(const VHBDataStream &other) {
        data = other.data;
        codec = other.codec;
        codec.reset();
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
        offset = 0;
    }

    TData get() override {
        return cur;
    }

    void next() override {
        offset = codec.getOffset();
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

    CompressedDataStream<TData>* copy() override {
        return new VHBDataStream<TData>(*this);
    }

    unsigned int getOffset() override {
        return offset;
    };

    void setOffset(unsigned int offset) override {
        this->offset = offset;
        codec.setOffset(offset);
        isEnd = codec.end();
        if (!isEnd) cur = codec.decodeNext();
    };
};
