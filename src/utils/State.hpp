#pragma once

#include <functional>

template <typename T> struct MutableState;

template <typename T> struct State {
  State(MutableState<T> &state) : mState{state} {}

  virtual void observe(std::function<void(T const &)> callback) {
    mState.mCallback = callback;
  }

private:
  MutableState<T> &mState;
};

template <typename T> struct MutableState : public State<T> {
  friend class State<T>;

  MutableState() : State<T>(*this) {}

  void observe(std::function<void(T const &)> callback) override {
    mCallback = callback;
  }

  void notify(T const &data) { mCallback(data); }

private:
  std::function<void(T const &)> mCallback;
};
