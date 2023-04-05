/*
 * The Priority Task Scheduler
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 1
 */

#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * A Multiple Queue priority scheduling algorithm
 */
class MultipleQueuePriorityScheduler : public SchedulingAlgorithm
{
public:
    /**
     * Returns the friendly name of the algorithm, for debugging and selection purposes.
     */
    const char* name() const override { return "mq"; }

    /**
     * Called during scheduler initialisation.
     */
    void init()
    {
        // TODO: Implement me! nah

    }

    /**
     * Called when a scheduling entity becomes eligible for running.
     * @param entity
     */
    void add_to_runqueue(SchedulingEntity& entity) override
    {
        /**
         * enqueue scheduling entity in its respective priority queue
         * 
         * This isnt the most extensible if you were to add lots of levels of priority queues,
         * as is the case in most OSs
         */
        if (entity.priority() == SchedulingEntityPriority::REALTIME) {
            UniqueIRQLock l;
		    realtime.enqueue(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::INTERACTIVE) {
            UniqueIRQLock l;
		    interactive.enqueue(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::NORMAL) {
            UniqueIRQLock l;
		    normal.enqueue(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::DAEMON) {
            UniqueIRQLock l;
		    daemon.enqueue(&entity);
        }
        else {
            syslog.messagef(LogLevel::ERROR, "scheduling entity has no valid priority");
        }
    }

    /**
     * Called when a scheduling entity is no longer eligible for running.
     * @param entity
     */
    void remove_from_runqueue(SchedulingEntity& entity) override
    {
        /**
         * remove scheduling entity from its respective priority queue
         */
        if (entity.priority() == SchedulingEntityPriority::REALTIME) {
            UniqueIRQLock l;
		    realtime.remove(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::INTERACTIVE) {
            UniqueIRQLock l;
		    interactive.remove(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::NORMAL) {
            UniqueIRQLock l;
		    normal.remove(&entity);
        }
        else if (entity.priority() == SchedulingEntityPriority::DAEMON) {
            UniqueIRQLock l;
		    daemon.remove(&entity);
        }
        else {
            // throw some error msg here
            syslog.messagef(LogLevel::ERROR, "scheduling entity has no valid priority");
        }
    }

    /**
     * Called every time a scheduling event occurs, to cause the next eligible entity
     * to be chosen.  The next eligible entity might actually be the same entity, if
     * e.g. its timeslice has not expired.
     */
    SchedulingEntity *pick_next_entity() override
    {
        SchedulingEntity *next_entity = NULL;
        // The lower priority queues can only be processed when the higher prio queues are empty.
        if (!realtime.empty()) {
            /**
            * If the queue is empty (and the queues higher prio than it are also empty)
            * then return the first elem of the queue and add this same elem to the back of the queue
            */
            UniqueIRQLock l;
            next_entity = realtime.dequeue();
            realtime.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "realtime");
        }
        else if (!interactive.empty()) {
            UniqueIRQLock l;
            next_entity = interactive.dequeue();
            interactive.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "interactive");
        }
        else if (!normal.empty()) {
            UniqueIRQLock l;
            next_entity = normal.dequeue();
            normal.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "normal");
        }
        else if (!daemon.empty()) {
            UniqueIRQLock l;
            next_entity = daemon.dequeue();
            daemon.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "daemon");
        }
        else {
            // If all queues empty return null
            return NULL;
        }

        return next_entity;
    }

private:
    List<SchedulingEntity *> realtime;
    List<SchedulingEntity *> interactive;
    List<SchedulingEntity *> normal;
    List<SchedulingEntity *> daemon;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(MultipleQueuePriorityScheduler);