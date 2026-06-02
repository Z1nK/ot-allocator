#pragma once
#include <cstddef>

template <typename T> struct Node {
  T data;
  Node *next;
};

template <typename T> class SimpleList {
public:
  SimpleList() : head_(nullptr), size_(0) {}

  ~SimpleList() {
    Node<T> *current = head_;
    while (current) {
      Node<T> *next = current->next;
      delete current;
      current = next;
    }
  }

  SimpleList(const SimpleList &) = delete;
  SimpleList &operator=(const SimpleList &) = delete;

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  void push_front(const T &value) {
    Node<T> *node = new Node<T>{value, head_};
    head_ = node;
    ++size_;
  }

  void push_back(const T &value) {
    Node<T> *node = new Node<T>{value, nullptr};
    if (!head_) {
      head_ = node;
    } else {
      Node<T> *tail = head_;
      while (tail->next) {
        tail = tail->next;
      }
      tail->next = node;
    }
    ++size_;
  }

  void pop_front() {
    if (!head_)
      return;
    Node<T> *old_head = head_;
    head_ = head_->next;
    delete old_head;
    --size_;
  }

  class Iterator {
  public:
    explicit Iterator(Node<T> *node) : current_(node) {}

    T &operator*() const { return current_->data; }
    Iterator &operator++() {
      if (current_)
        current_ = current_->next;
      return *this;
    }
    
    bool operator!=(const Iterator &other) const {
      return current_ != other.current_;
    }

  private:
    Node<T> *current_;
  };

  Iterator begin() { return Iterator(head_); }
  Iterator end() { return Iterator(nullptr); }

private:
  Node<T> *head_;
  std::size_t size_;
};
