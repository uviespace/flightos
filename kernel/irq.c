/**
 * @file kernel/irq.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup irqsys
 * @ingroup interrupts
 * @defgroup irqsys Interrupt subsystem
 *
 * @brief kernel IRQ interface (high-level, architecture-independent IRQ API)
 *
 * This is the high-level abstracted IRQ interface of the kernel. It contains
 * no hardware-specific logic of its own: the request, release, deferred
 * execution, affinity, and level entry points forward to operations of a
 * single, platform-provided struct irq_dev registered through irq_init().
 *
 * ## Concept
 *
 * The interrupt subsystem is split into two layers:
 *   - the high-level abstracted interface, implemented in this file and
 *     declared in include/kernel/irq.h;
 *   - a platform-specific backend under arch/ that implements the operations
 *     of struct irq_dev and wires it into the generic layer with
 *     irq_init(&backend_irq).
 *
 * Only a LEON (SPARCv8) backend exists so far: arch/sparc/kernel/irq.c
 * provides struct irq_dev leon_irq and registers it from leon_irq_init(),
 * which is called by setup_arch() during boot. The relationship between the
 * two layers and the architecture-side handling is documented by the
 * architecture section and diagrams on this group page.
 *
 * ## Structure
 *
 * The generic interface is built around a single static pointer:
 *
 *   - irq_ctrl            : the registered struct irq_dev; set by irq_init().
 *   - irq_init(dev)       : store the platform's struct irq_dev.
 *   - irq_request()       : request an IRQ line. Returns -EINVAL if irq_ctrl
 *                           or the irq_enable operation is unset, otherwise
 *                           builds a struct irq_data {irq, priority, handler,
 *                           data} and returns irq_ctrl->irq_enable().
 *   - irq_free()          : release an IRQ line. Returns -EINVAL if irq_ctrl
 *                           or the irq_disable operation is unset, otherwise
 *                           calls irq_ctrl->irq_disable() and returns 0.
 *   - irq_exec_deferred() : run pending deferred handlers through the
 *                           irq_deferred operation, if provided. Returns
 *                           -EINVAL without a registered irq_ctrl.
 *   - irq_set_affinity()  : route an IRQ to a CPU through the
 *                           irq_set_affinity operation, if provided. Returns
 *                           -EINVAL without a registered irq_ctrl.
 *   - irq_set_level()     : adjust the hardware priority of an IRQ through
 *                           the irq_set_level operation, if provided. Unlike
 *                           the other entry points it does not validate
 *                           irq_ctrl before using it (needs review).
 *
 * The C sources observed as consumers call the exported functions above. The
 * implementation does not enforce whether a caller accesses irq_ctrl or
 * struct irq_dev directly; direct-access policy needs review.
 *
 * @startuml FlightOS IRQ Subsystem
 * title IRQ Subsystem Dependencies
 *
 * !pragma layout ortho
 *
 * skinparam component {
 *   BackgroundColor #FFEBEE
 *   BorderColor #333333
 * }
 * skinparam arrowColor #333333
 *
 * package "IRQ Consumers" {
 *   [clockevent.c\n(arch)] as CLKEVT
 *   [tick.c] as TICK
 *   [watchdog.c] as WDOG
 *   [noc_dma.c] as NOC_DMA
 *   [grspw2.c] as GRSPW2
 *   [grspi.c] as GRSPI
 *   [xentium.c] as XENTIUM
 *   [edac.c\n(arch)] as EDAC
 *   [projects/*.c] as PROJ
 * }
 *
 * package "IRQ Core (kernel/)" {
 *   [irq.c] as IRQ
 * }
 *
 * package "Architecture" {
 *   [irq.c\n(sparc)] as IRQARCH
 *   [irqtrap.S] as IRQTRAP
 *   [leon irqctrl\nregisters] as IRQHW
 * }
 *
 * package "Abstraction" {
 *   [struct irq_dev] as IRQ_DEV
 *   [struct irq_data] as IRQ_DATA
 * }
 *
 * CLKEVT -down-> IRQ : irq_request()\nirq_set_level()
 * TICK -down-> IRQ : irq_set_affinity()
 * WDOG -down-> IRQ : irq_set_affinity()
 * NOC_DMA -down-> IRQ : irq_request()
 * GRSPW2 -down-> IRQ : irq_request()\nirq_free()
 * GRSPI -down-> IRQ : irq_request()
 * XENTIUM -down-> IRQ : irq_request()
 * EDAC -down-> IRQ : irq_request()
 * PROJ -down-> IRQ : irq_request()\nirq_set_affinity()\nirq_set_level()
 *
 * IRQ -down-> IRQ_DEV : generic API\n(via irq_ctrl)
 * IRQ -down-> IRQ_DATA : request parameters
 * IRQARCH -up-> IRQ_DEV : implements
 * IRQARCH -right-> IRQTRAP : leon_irq_dispatch()
 * IRQTRAP -right-> IRQHW : trap entry
 * IRQARCH -down-> IRQHW : controller registers
 *
 * note bottom of IRQ_DEV
 *   irq_dev ops implemented by the LEON backend:
 *   irq_enable / irq_disable
 *   irq_mask / irq_unmask
 *   irq_deferred
 *   irq_set_affinity
 *   irq_set_level
 * end note
 *
 * @enduml
 *
 * @startuml FlightOS IRQ Generic API Flow
 * title Generic IRQ API Flow
 *
 * skinparam activityBackgroundColor #FFF3E0
 * skinparam activityBorderColor #333333
 *
 * |irq_request()|
 * start
 * :validate irq_ctrl and\nirq_enable operation;
 * if (missing) then (yes)
 *   :return -EINVAL;
 * else (ok)
 *   :build struct irq_data\n{irq, priority, handler, data};
 *   :irq_ctrl->irq_enable(&cfg);
 * endif
 * :return value from backend;
 * stop
 *
 * |irq_free()|
 * start
 * :validate irq_ctrl and\nirq_disable operation;
 * if (missing) then (yes)
 *   :return -EINVAL;
 * else (ok)
 *   :build struct irq_data\n{irq, handler, data};
 *   :irq_ctrl->irq_disable(&cfg);
 * endif
 * :return 0;
 * stop
 *
 * |irq_exec_deferred()|
 * start
 * if (irq_ctrl missing) then (yes)
 *   :return -EINVAL;
 * else (registered)
 *   if (irq_deferred op set) then (yes)
 *     :irq_ctrl->irq_deferred();
 *   else (op unset)
 *   endif
 * endif
 * :return 0;
 * stop
 *
 * |irq_set_affinity()|
 * start
 * if (irq_ctrl missing) then (yes)
 *   :return -EINVAL;
 * else (registered)
 *   if (irq_set_affinity\nop set) then (yes)
 *     :irq_ctrl->irq_set_affinity(irq, cpu);
 *   else (op unset)
 *   endif
 * endif
 * :return 0;
 * stop
 *
 * |irq_set_level()|
 * start
 * if (irq_set_level\nop set) then (yes)
 *   :irq_ctrl->irq_set_level(irq, priority);
 *   note right
 *     needs review: irq_ctrl is
 *     not validated before use
 *   end note
 * else (op unset) then (no)
 * endif
 * stop
 *
 * |irq_init()|
 * start
 * :irq_ctrl = dev;
 * stop
 *
 * @enduml
 */


#include <errno.h>
#include <kernel/irq.h>
#include <kernel/export.h>

static struct irq_dev *irq_ctrl;


/**
 * @brief request a slot for an interrupt handler
 *
 * @param irq      the interrupt number to register
 * @param priority the ISR execution priority
 * @param handler  the interrupt handler function
 * @param data     opaque data pointer passed to the handler
 *
 * @return 0 on success, negative errno on error
 */

int irq_request(unsigned int irq, enum isr_exec_priority priority,
		irq_handler_t handler, void *data)
{
	struct irq_data cfg;


	if (!irq_ctrl)
		return -EINVAL;

	if (!irq_ctrl->irq_enable)
		return -EINVAL;

	cfg.irq      = irq;
	cfg.priority = priority;
	cfg.handler  = handler;
	cfg.data     = data;

	return irq_ctrl->irq_enable(&cfg);
}
EXPORT_SYMBOL(irq_request);

/**
 * @brief release an interrupt handler
 *
 * @param irq     the interrupt number to release
 * @param handler the handler function that was registered
 * @param data    the data pointer that was passed during registration
 *
 * @return 0 on success, negative errno on error
 */

int irq_free(unsigned int irq, irq_handler_t handler, void *data)
{
	struct irq_data cfg;


	if (!irq_ctrl)
		return -EINVAL;

	if (!irq_ctrl->irq_disable)
		return -EINVAL;

	cfg.irq      = irq;
	cfg.handler  = handler;
	cfg.data     = data;

	irq_ctrl->irq_disable(&cfg);

	return 0;
}
EXPORT_SYMBOL(irq_free);


/**
 * @brief execute deferred interrupt handlers
 *
 * @return 0 on success, negative errno on error
 */

int irq_exec_deferred(void)
{
	if (!irq_ctrl)
		return -EINVAL;

	if (irq_ctrl->irq_deferred)
		irq_ctrl->irq_deferred();

	return 0;
}
EXPORT_SYMBOL(irq_exec_deferred);


/**
 * @brief set CPU affinity to a particular CPU (in SMP)
 *
 * @param irq the interrupt number
 * @param cpu the target CPU number
 *
 * @return 0 on success, negative errno on error
 */

int irq_set_affinity(unsigned int irq, int cpu)
{
	if (!irq_ctrl)
		return -EINVAL;

	if (irq_ctrl->irq_set_affinity)
		irq_ctrl->irq_set_affinity(irq, cpu);

	return 0;

}
EXPORT_SYMBOL(irq_set_affinity);


/**
 * @brief set an interrupt priority level
 *
 * @param irq      the interrupt number
 * @param priority the priority level to set
 */

void irq_set_level(unsigned int irq, unsigned int priority)
{
	if (irq_ctrl->irq_set_level)
		irq_ctrl->irq_set_level(irq, priority);
}
EXPORT_SYMBOL(irq_set_level);


/**
 * @brief initialise the IRQ system
 *
 * @param dev pointer to the irq_dev structure for the platform
 */

void irq_init(struct irq_dev *dev)
{
	irq_ctrl = dev;
}
