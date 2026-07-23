
namespace Ogl
{
    template <class T>
    using EventHandler = bool(*)(T&, void*); //accepts event & data, returns whether event has been handled (if it has been then other handlers won't be called)

    template <class T>
    struct Subscription
    {
        EventHandler<T> Handler;
        int Priority; //handlers with higher priority will be called first
        void* Data;

        bool operator==(const Subscription<T>& x) const
        {
            return Priority == x.Priority && Handler == x.Handler && Data == x.Data;
        }

        bool operator>(const Subscription<T>& x) const
        {
            if (Priority == x.Priority)
            {
                if (Handler == x.Handler)
                    return Data > x.Data;

                return Handler > x.Handler;
            }

            return Priority > x.Priority;
        }
    };

    template <class T>
    struct std::hash<Ogl::Subscription<T>>
    {
        std::size_t operator()(const Ogl::Subscription<T>& sub) const noexcept
        {
            return ((std::hash<void*>()(sub.Handler) ^ (std::hash<void*>()(sub.Data) << 1)) >> 1);
        }
    };

    template <class T>
    using SubscriptionSet = std::set<Subscription<T>, std::greater<Subscription<T>>>;

    //sub/unsub methods

    template <class T>
    SubscriptionSet<T>& GetSubscriptions()
    {
        static SubscriptionSet<T> subs;
        return subs;
    }

    //subscribes 'handler' to the event T
    //if 'layer' is set an unsubscription handler will be created and called automatically upon layer's destruction, otherwise 'Unsubscribe' should be called manually
    //if 'data' is not null it will be passed to the handler, otherwise 'layer' will be passed instead
    //handlers with higher 'priority' will get called first
    template <class T>
    const Subscription<T>& Subscribe(EventHandler<T> handler, void* data = nullptr, int priority = 0)
    {
        auto pair = GetSubscriptions<T>().insert({ handler, priority, data });
        if (!pair.second)
            throw std::runtime_error("Failed to insert new subscription, maybe an identical one already exists.");

        const Subscription<T>& sub = *pair.first;
        return sub;
    }

    template <class T>
    void Unsubscribe(const Subscription<T>& sub)
    {
        GetSubscriptions<T>().erase(sub);
    }

    //invokes event T, calling handlers with higher priority first
    template <class T>
    void Invoke(T& event)
    {
        for (Subscription sub : GetSubscriptions<T>())
        {
            if (sub.Handler(event, sub.Data))
                return;
        }
    }

    //subscriber

    struct Subscriber
    {
        std::vector<std::function<void()>> UnsubHandlers; //lambdas wrapped in 'std::function' (can't use regular function pointers here) which will be called upon subscriber's destruction to unsubscribe it from any events
        int DefaultPriority = 0;

        Subscriber() {}

        //no copies
        Subscriber(const Subscriber& ref) = delete;
        void operator=(const Subscriber& ref) = delete;
        
        //identical to the other 'Subscribe' method but passes layer to the handler and automatically adds an unsub callback 
        //which will be called on layer's destruction
        template <class T>
        void Subscribe(EventHandler<T> handler, int priority)
        {
            Subscription<T> sub = Ogl::Subscribe(handler, this, priority);
            UnsubHandlers.push_back(std::function<void()>([sub]() { Ogl::Unsubscribe(sub); }));
        }

        template <class T>
        void Subscribe(EventHandler<T> handler)
        {
            Subscribe(handler, DefaultPriority);
        }

        virtual ~Subscriber()
        {
            for (std::function<void()> handler : UnsubHandlers)
            {
                handler();
            }
        }
    };
}
