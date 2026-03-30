#pragma once
#include <string>

class IMessage {
public:
    virtual ~IMessage() = default;
    virtual std::string getType() const = 0;
    virtual const std::string& raw() const = 0;
};

// ----------------------------

class JsonMessage : public IMessage {
public:
    JsonMessage(std::string type, std::string raw)
        : type_(std::move(type)), raw_(std::move(raw)) {}

    std::string getType() const override {
        return type_;
    }

    const std::string& raw() const override {
        return raw_;
    }

private:
    std::string type_;
    std::string raw_;
};