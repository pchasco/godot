#ifndef FASTVECTOR_H
#define FASTVECTOR_H

// Never set to less than 1 because pop() returns _data[0] if nothing in vector
#define STACK_MIN_BUFFER_SIZE 8

template <typename T>
class FastVector {
public:
	FastVector() : FastVector(STACK_MIN_BUFFER_SIZE) {}

	FastVector(size_t p_initial_buffer_size) {
		if (p_initial_buffer_size == 0) {
			p_initial_buffer_size = STACK_MIN_BUFFER_SIZE;
		}

		_top = 0;
		_size = p_initial_buffer_size;
		_data = nullptr;
	}

	FastVector(const FastVector<T>& other)
		: FastVector(other._top)
	{
		_top = other._top;
		
		initialize_data();
		
		for (int i = 0; i < _top; ++i) {
			new (_data + i, sizeof(T), "") T(other._data[i]);
		}
	}

	virtual ~FastVector() {
		if (_data != nullptr) {
			clear();
			memfree(_data);
			_data = nullptr;
			_size = 0;
		}
	}

	FastVector<T>& operator=(const FastVector<T>& other) {
		_size = other._size;

		size_t bytes = sizeof(T) * _size;
		if (_data == nullptr) {
			_data = (T*)memalloc(bytes);
		} else {
			clear();
			_data = (T*)memrealloc(_data, bytes);
		}

		_top = other._top;

		for (int i = 0; i < _top; ++i) {
			new (_data + i, sizeof(T), "") T(other._data[i]);
		}

		return *this;
	}

	void push(const T& p_value) {
		if (_top == _size || _data == nullptr) {
			if (_top == _size) {
				_size *= 2;
				if (_size < STACK_MIN_BUFFER_SIZE) {
					_size = STACK_MIN_BUFFER_SIZE;
				}
			}

			if (_data == nullptr) {
				_data = (T*)memalloc(sizeof(T) * _size);
			} else {
				_data = (T*)memrealloc(_data, sizeof(T) * _size);
			}
		}

		new (_data + _top, sizeof(T), "") T(p_value);
		_top += 1;
	}

	void push_many(int count, const T* p_values) {
		for (int i = 0; i < count; ++i) {
			push(p_values[i]);
		}
	}

	void push_many(const FastVector<T> &vector) {
		for (int i = 0; i < vector._top; ++i) {
			push(vector._data[i]);
		}
	}

	T pop() {
		if (_top <= 0) {
			ERR_FAIL_V_MSG(*((T*)nullptr), "Tried to pop when nothing to pop");
		}

		return _data[--_top];
	}

	bool empty() const {
		return _top == 0;
	}

	T *address_of(int p_index) const {
		if (p_index < 0 || p_index >= _top) {
			ERR_FAIL_V_MSG(nullptr, "Index out of range");
		}

		return &_data[p_index];
	}

	int size() const {
		return _top;
	}

	T &operator[](int p_index) const {
		if (p_index < 0 || p_index >= _top) {
			ERR_FAIL_V_MSG(*((T*)nullptr), "Index out of range");
		}

		return _data[p_index];
	}

	int index_of(const T &p_value) const {
		for (int i = 0; i < _top; ++i) {
			if (_data[i] == p_value) {
				return i;
			}
		}

		return -1;
	}

	bool has(const T &p_value) const {
		return index_of(p_value) >= 0;
	}

	void clear() {
		if (_data != nullptr) {
			for (int i = 0; i < _top; ++i) {
				_data[i].~T();
			}
		}
		
		_top = 0;
	}

	const T *ptr() const {
		return _data;
	}

private:
	void initialize_data() {		
		size_t bytes = sizeof(T) * _size;
		_data = (T*)memalloc(bytes);
	}

	T *_data;
	int _size;
	int _top;
};

#endif