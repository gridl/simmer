#ifndef simmer__process_batched_h
#define simmer__process_batched_h

#include <simmer/process/arrival.h>

namespace simmer {

  /**
   *  Batch of arrivals.
   */
  class Batched : public Arrival {
  public:
    CLONEABLE(Batched)

    Batched(Simulator* sim, const std::string& name, bool permanent, int priority = 0)
      : Arrival(sim, name, true, Order(), NULL, priority), permanent(permanent) {}

    Batched(const Batched& o) : Arrival(o), arrivals(o.arrivals), permanent(o.permanent) {
      for (size_t i=0; i<arrivals.size(); i++) {
        arrivals[i] = arrivals[i]->clone();
        arrivals[i]->register_entity(this);
      }
    }

    ~Batched() { reset(); }

    void terminate(bool finished) {
      foreach_ (Arrival* arrival, arrivals)
      arrival->terminate(finished);
      arrivals.clear();
      Arrival::terminate(finished);
    }

    bool pop_all(Activity* next) {
      if (permanent) return false;
      foreach_ (Arrival* arrival, arrivals) {
        arrival->set_activity(next);
        arrival->unregister_entity(this);
        arrival->activate();
      }
      arrivals.clear();
      delete this;
      return true;
    }

    void set_attribute(const std::string& key, double value, bool global=false) {
      if (global) return sim->set_attribute(key, value);
      attributes[key] = value;
      foreach_ (Arrival* arrival, arrivals)
        arrival->set_attribute(key, value);
    }

    size_t size() const { return arrivals.size(); }

    void insert(Arrival* arrival) {
      arrival->set_activity(NULL);
      arrivals.push_back(arrival);
      arrival->register_entity(this);
    }

    bool erase(Arrival* arrival) {
      if (permanent) return false;
      bool del = activity;
      if (arrivals.size() > 1 || (batch && batch->permanent)) {
        del = false;
        if (arrival->is_monitored()) {
          Batched* up = this;
          while (up) {
            up->report(arrival);
            up = up->batch;
          }
        }
      } else if (arrivals.size() == 1 && !batch) {
        if (!leave_resources(!activity))
          deactivate();
      } else {
        del = true;
        batch->erase(this);
        leave_resources();
      }
      arrivals.erase(std::remove(arrivals.begin(), arrivals.end(), arrival), arrivals.end());
      arrival->unregister_entity(this);
      if (del) delete this;
      return true;
    }

  private:
    VEC<Arrival*> arrivals;
    bool permanent;

    void reset() {
      foreach_ (Arrival* arrival, arrivals)
        delete arrival;
      arrivals.clear();
    }

    void report(const std::string& resource) const {
      foreach_ (const Arrival* arrival, arrivals) {
        if (arrival->is_monitored()) {
          ArrTime time = restime.find(resource)->second;
          arrival->report(resource, time.start, time.activity);
        }
      }
    }

    void report(const std::string& resource, double start, double activity) const {
      foreach_ (const Arrival* arrival, arrivals) {
        if (arrival->is_monitored())
          arrival->report(resource, start, activity);
      }
    }

    void report(Arrival* arrival) const {
      foreach_ (const ResTime::value_type& itr, restime)
      arrival->report(itr.first, itr.second.start,
                      itr.second.activity - status.busy_until + sim->now());
    }

    void update_activity(double value) {
      Arrival::update_activity(value);
      foreach_ (Arrival* arrival, arrivals)
        arrival->update_activity(value);
    }

    void set_remaining(double value) {
      Arrival::set_remaining(value);
      foreach_ (Arrival* arrival, arrivals)
        arrival->set_remaining(value);
    }

    void set_busy(double value) {
      Arrival::set_busy(value);
      foreach_ (Arrival* arrival, arrivals)
        arrival->set_busy(value);
    }
  };

} // namespace simmer

#endif
