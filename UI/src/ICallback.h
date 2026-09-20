#ifndef ICALLBACK_H
#define ICALLBACK_H

class ICallback
{
public:
    virtual void Execute() = 0;
    virtual ~ICallback(){}

    void set_arg(void *arg) { usr_arg = arg; }
    void * get_arg() { return usr_arg; }

private:
    void* usr_arg;
};

template <class Receiver>
class SimpleCallback : public ICallback
{
public:
    typedef void (Receiver::* Action)();

    SimpleCallback(Receiver* r, Action a)
        : _receiver(r), _action(a) { }

    virtual void Execute();

private:
    Receiver* _receiver;
    Action _action;


};

template <class Receiver>
void SimpleCallback<Receiver>::Execute ()
{
    (_receiver->*_action)();
}


#endif // ICALLBACK_H
