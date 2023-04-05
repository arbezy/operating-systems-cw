#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * Weighted round robin schedling algorithm
 */
class AdvancedPriorityScheduler : public SchedulingAlgorithm
{
public:
	/**
	 * Returns the friendly name of the algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "adv"; }

    /**
     * Called during scheduler initialisation.
     */
    void init()
    {
        current_queue = 0;
        allocated_time = 8;
        cumulative_queue_runtime = 0;
    }
	
	/**
	 * Called when a scheduling entity becomes eligible for running.
	 * @param entity
	 */
	void add_to_runqueue(SchedulingEntity& entity) override
	{
        /**
         * enqueue scheduling entity in its respective priority queue
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
	 * to be chosen.
	 */
	SchedulingEntity *pick_next_entity() override
	{	

        SchedulingEntity *next_entity = NULL;

        // increment the cumulative runtime for the queue currently using the CPU
        cumulative_queue_runtime += 1;
        //syslog.messagef(LogLevel::DEBUG, "cumulative runtime: %d", cumulative_queue_runtime);
        //syslog.messagef(LogLevel::DEBUG, "allocated runtime: %d", allocated_time);

        if (current_queue == 0 && !realtime.empty()) {
            /**
            * If the queue is selected and not empty:
            * then return the first elem of the queue and add this same elem to the back of the queue
            * (round robin style)
            */
            UniqueIRQLock l;
            next_entity = realtime.dequeue();
            realtime.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "realtime");
        }
        else if (current_queue == 1 && !interactive.empty()) {
            UniqueIRQLock l;
            next_entity = interactive.dequeue();
            interactive.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "interactive");
        }
        else if (current_queue == 2 && !normal.empty()) {
            UniqueIRQLock l;
            next_entity = normal.dequeue();
            normal.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "normal");
        }
        else if (current_queue == 3 && !daemon.empty()) {
            UniqueIRQLock l;
            next_entity = daemon.dequeue();
            daemon.enqueue(next_entity);
            //syslog.messagef(LogLevel::DEBUG, "daemon");
        }

        // if allotted time exceeded then move onto the next queue and reset cumulative queue run time
        if (allocated_time < cumulative_queue_runtime) {
            //syslog.messagef(LogLevel::DEBUG, "allocated runtime exceeded, switching queues");
            current_queue = (current_queue + 1) % 4;
            cumulative_queue_runtime = 0;
            // reduces allocated time by half or if less than or equal to one, reset it to 8
            // allocated time goes 8 > 4 > 2 > 1 for each level of priority
            if (allocated_time <= 1) {
                allocated_time = 8;
            } 
            else {
                allocated_time /= 2;
            }
        }

        return next_entity;

	}
	
private:
    int current_queue;
    // times in multiples of 10ms
    // theses current times could probs be tweaked so higher prio threads more favoured
    int allocated_time; //80ms, 40ms, 20ns, 10ns (8,4,2,1)
    int cumulative_queue_runtime;
	List<SchedulingEntity *> realtime;
    List<SchedulingEntity *> interactive;
    List<SchedulingEntity *> normal;
    List<SchedulingEntity *> daemon;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(AdvancedPriorityScheduler);