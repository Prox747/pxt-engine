#ifndef PXTENGINE_EVENT_H // Standard include guard - Start
#define PXTENGINE_EVENT_H

#pragma once // Keep for convenience, but standard guards ensure wider compatibility

#include <string>
#include <functional> // Often useful for event dispatching/callbacks later
#include <sstream>    // Useful for more detailed toString in derived classes
#include <chrono>     // For adding timestamps

namespace PXTEngine {

    // Forward declaration for dispatcher example comment
    class EventDispatcher;

    /**
     * @enum EventCategory
     * @brief Bit flags representing event categories. Allows efficient filtering.
     *
     * Events can belong to multiple categories (e.g., a MouseButtonPress event
     * is in Input, Mouse, and MouseButton categories).
     */
    enum EventCategory : int {
        None          = 0,
        Application   = 1 << 0, // 1 - Core application events (tick, update, etc.)
        Input         = 1 << 1, // 2 - General input category
        Keyboard      = 1 << 2, // 4 - Keyboard input
        Mouse         = 1 << 3, // 8 - Mouse input (movement, buttons, scroll)
        MouseButton   = 1 << 4, // 16 - Specific mouse button events
        Window        = 1 << 5, // 32 - Windowing system events (resize, close, focus)
        // Add more categories as needed: Network, Physics, UI, Scene, Custom, etc.
    };

    // Define bitwise OR operator for combining category flags easily
    inline EventCategory operator|(EventCategory a, EventCategory b) {
        return static_cast<EventCategory>(static_cast<int>(a) | static_cast<int>(b));
    }
    // Define bitwise AND operator for checking category membership (returns non-zero if category matches)
    inline int operator&(EventCategory a, EventCategory b) {
        return static_cast<int>(a) & static_cast<int>(b);
    }

    /**
     * @class Event
     * @brief Base class for all event types in the engine.
     *
     * Provides an interface for getting the event type, name, category, timestamp,
     * and handling status. Derived classes should use the provided macros
     * (EVENT_CLASS_TYPE, EVENT_CLASS_CATEGORY) to reduce boilerplate code.
     *
     * Events are typically processed via an EventDispatcher.
     */
    class Event {
    public:
        /**
         * @enum Type
         * @brief Enumeration of specific event types.
         */
        enum class Type {
            None = 0,

            // --- Window events --- Category: Window | Application ---
            WindowClose,
            WindowResize,
            WindowMoved,
            WindowFocus,
            WindowLostFocus,

            // --- Application events --- Category: Application ---
            AppTick,        // Example: Fixed timestep update
            AppUpdate,      // Example: Variable timestep update
            AppRender,      // Example: Render call event

            // --- Keyboard events --- Category: Keyboard | Input ---
            KeyPress,       // Key pressed down (potentially repeating if held, depends on system)
            KeyRelease,     // Key released (consider renaming to KeyUp for pair consistency)
            KeyTyped,       // Character input event (respects layout, modifiers like Shift)

            // --- Mouse events --- Category: Mouse | Input ---
            MouseButtonPress,   // Category: MouseButton | Mouse | Input
            MouseButtonRelease, // Category: MouseButton | Mouse | Input
            MouseMove,
            MouseScroll,
        };

        // Virtual destructor is crucial for base classes with virtual functions.
        virtual ~Event() = default;

        // --- Core Event Information ---

        /** @brief Returns the specific type of this event instance. */
        virtual Type GetEventType() const = 0;

        /**
         * @brief Returns the constant name of the event type (primarily for debugging).
         * @return A C-style string literal representing the event name (e.g., "WindowClose").
         */
        virtual const char* GetName() const = 0; // Use const char* for efficient string literals

        /** @brief Returns the category bit flags for this event instance. */
        virtual int GetCategoryFlags() const = 0;

        /**
         * @brief Returns a string representation of the event.
         * Derived classes can override this for more detailed debug information.
         * @return A std::string describing the event.
         */
        virtual std::string ToString() const { return GetName(); }

        // --- Event Handling Status ---

        /** @brief Checks if this event has been handled by a listener. Handled events might be ignored by subsequent layers. */
        bool IsHandled() const { return m_handled; }

        /**
         * @brief Marks the event as handled or unhandled.
         * @param handled Set to true to mark as handled (default), false to unmark.
         */
        void SetHandled(bool handled = true) { m_handled = handled; }

        // --- Category Check ---

        /**
         * @brief Checks if the event belongs to a specific category using bitwise AND.
         * @param category The category flag to check against.
         * @return True if the event's flags include the specified category flag, false otherwise.
         */
        bool IsInCategory(EventCategory category) const {
            // Returns non-zero (true) if the category bit is set in the event's flags
            return GetCategoryFlags() & category;
        }

        // --- Timestamp ---

        /** @brief Gets the time point when the event was created. Uses a steady clock for monotonic time. */
        std::chrono::steady_clock::time_point GetTimestamp() const { return m_timestamp; }

    protected:
        // Protected constructor initializes the timestamp when an event object is created.
        // Derived classes implicitly call this.
        Event() : m_timestamp(std::chrono::steady_clock::now()) {}

        // Friend declaration allows the dispatcher direct access if needed,
        // though using public SetHandled is often sufficient and simpler.
        // friend class EventDispatcher;

    private:
        bool m_handled = false;
        std::chrono::steady_clock::time_point m_timestamp;

    }; // class Event


    // --- Macros for Implementing Derived Event Classes ---
    // These significantly reduce boilerplate code in classes inheriting from Event.

    /**
     * @def EVENT_CLASS_TYPE
     * @brief Implements GetStaticType(), GetEventType(), and GetName() for a derived event class.
     * @param type The specific Event::Type enum value for the derived class (e.g., WindowClose).
     *
     * Example Usage in derived class header:
     * @code
     * class WindowCloseEvent : public Event {
     * public:
     * WindowCloseEvent() = default;
     * EVENT_CLASS_TYPE(WindowClose)
     * EVENT_CLASS_CATEGORY(EventCategory::Window | EventCategory::Application)
     * };
     * @endcode
     */
    #define EVENT_CLASS_TYPE(type)  public: \
                                        static Event::Type GetStaticType() { return Event::Type::type; } \
                                        virtual Event::Type GetEventType() const override { return GetStaticType(); } \
                                        virtual const char* GetName() const override { return #type; } \
                                    private: /* Avoid accidental slicing or direct instantiation if needed */

    /**
     * @def EVENT_CLASS_CATEGORY
     * @brief Implements GetCategoryFlags() for a derived event class.
     * @param category The EventCategory bit flags for the derived class (e.g., EventCategory::Mouse | EventCategory::Input).
     *
     * Example Usage (see EVENT_CLASS_TYPE example).
     */
    #define EVENT_CLASS_CATEGORY(category) public: \
                                                virtual int GetCategoryFlags() const override { return category; } \
                                            private:

    // --- Optional: Inline Event Dispatcher Helper ---
    // Provides a convenient way to dispatch events based on their type.
    // Place this here or in a separate EventDispatcher.h file.
    class EventDispatcher {
    public:
        // Takes a reference to the event to be dispatched.
        EventDispatcher(Event& event) : m_event(event) {}

        // Attempts to dispatch the event to a handler function if the event type matches T.
        // The handler function (func) should take a const T& or T& and return bool (true if handled, false otherwise).
        template<typename T, typename F>
        bool Dispatch(const F& func) {
            // Check if the event instance's type matches the static type of T provided by the macro.
             if constexpr (requires { T::GetStaticType(); }) { // Check if T implements GetStaticType (via macro)
                if (m_event.GetEventType() == T::GetStaticType()) {
                    // If the event hasn't already been handled, call the function.
                    if (!m_event.IsHandled()) {
                        // Call the handler, casting the event reference to the specific derived type T&.
                        // The handler's return value (true/false) determines if the event is now handled.
                        m_event.SetHandled(func(static_cast<T&>(m_event)));
                    }
                    return true; // Indicate that a handler matching this type T was found (even if event was already handled).
                }
            } else {
                 // Compile-time error if the macro wasn't used correctly in the event class T.
                 static_assert(sizeof(T) == 0, "EVENT_CLASS_TYPE macro not used or GetStaticType() not implemented for the specified event type T.");
            }
            return false; // Indicate no handler matching type T was found.
        }

    private:
        Event& m_event;
    };

} // namespace PXTEngine

#endif // PXTENGINE_EVENT_H - Standard include guard - End
