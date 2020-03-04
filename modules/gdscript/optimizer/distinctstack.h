#ifndef DISTINCTSTACK_H
#define DISTINCTSTACK_H

#include "core/os/memory.h"
#include "fastvector.h"

template <typename T>
class DistinctStack {
public:
    void push(const T& p_value) {
        if (!_history.has(p_value)) {
            _vector.push(p_value);
            _history.insert(p_value);
        }
    }

	void push_many(unsigned int p_count, const T* p_values) {
		for (int i = 0; i < p_count; ++i) {
            push(p_values[i]);
        }
	}

	void push_many(const FastVector<T> &p_vector) {
		for (int i = 0; i < p_vector.size(); ++i) {
			push(p_vector[i]);
		}
	}

    T pop() {
        return _vector.pop();
    }
    
    T& peek() const {
        return _vector[_vector.size() - 1];
    }

	bool empty() const {
		return _vector.size() == 0;
	}

	int size() const {
		return _vector.size();
	}

    void clear(bool retain_history) {
        _vector.clear();
        if (!retain_history) {
            _history.clear();
        }
    }

    void clear() {
        clear(false);
    }

private:
    FastVector<T> _vector;
    Set<T> _history;
};

#endif