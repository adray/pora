# Pora 

Pora is an experimental programming language which aims to be streamlined and aesthetically pleasing. It is multi paradigm namely, OOP and producural. It is a manually memory managed language, statically typed and AOT compiled. It is written in C++ with no other external dependicies - other than CMake used for the build system.

Here is an example of the clock class found in std\win\clock.po. 
```
namespace std
{
    struct clock_time {
        u8[9] text;
    }

    class clock {
        private u16 _seconds;
        private u16 _minutes;
        private u16 _hours;
        
        clock();
        void setNow();
        void reset();
        void addSeconds(u16 seconds);
        void addMinutes(u16 minutes);
        void addHours(u16 hours);
        u16 getSecond => _seconds;
        u16 getMinute => _minutes;
        u16 getHour => _hours;
        void getTime(clock_time* time);
        private void update();
    }
    
    clock::clock() {
        _seconds = (u16)0;
        _minutes = (u16)0;
        _hours = (u16)0;
    }
    
    void clock::setNow() {
        SYSTEMTIME time;
        GetLocalTime(&time);
        
        _seconds = time.wSecond;
        _minutes = time.wMinute;
        _hours = time.wHour;
    }
    
    void clock::reset() {
        _seconds = (u16)0;
        _minutes = (u16)0;
        _hours = (u16)0;
    }
    
    void clock::update() {
        if (_seconds >= (u16)60) {
            _seconds -= (u16)60;
            _minutes += (u16)1;
        }
        if (_minutes >= (u16)60) {
            _minutes -= (u16)60;
            _hours += (u16)1;
        }
        if (_hours >= (u16)24) {
            _hours -= (u16)24;
        }
    }
    
    void clock::addSeconds(u16 seconds) {
        _seconds += seconds;
        update();
    }
    
    void clock::addMinutes(u16 minutes) {
        _minutes += minutes;
        update();
    }
    
    void clock::addHours(u16 hours) {
        _hours += hours;
        update();
    }
    
    // Formats the time in text format.
    void clock::getTime(clock_time* time) {
        if (time == null) {
            return;
        }
        
        time.text[0] = '0' + (u8)(_hours / (u16)10);
        time.text[1] = '0' + (u8)(_hours % (u16)10);        
        time.text[2] = ':';
        
        time.text[3] = '0' + (u8)(_minutes / (u16)10);
        time.text[4] = '0' + (u8)(_minutes % (u16)10);
        time.text[5] = ':';
       
        time.text[6] = '0' + (u8)(_seconds / (u16)10);
        time.text[7] = '0' + (u8)(_seconds % (u16)10);
        time.text[8] = 0b;
    }
}

```

Generics are supported, here is a sample from the map class.

```
    class map<K:new, V:new, H:hash<K>> {
        private K* _keys;
        private V* _values;
        private i64* _hashes;
        private boolean* _occupied;
        private i64 _capacity;
        private i64 _size;

        map();
        void insert(K key, V val);
        boolean find(K key, V* val);
        boolean erase(K key);
        void clear();

        i64 size => _size;
        i64 capacity => _capacity;

        private void resize(i64 capacity);
    }
```

