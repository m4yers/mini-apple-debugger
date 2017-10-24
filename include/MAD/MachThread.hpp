#ifndef MACHTHREAD_HPP_INMSKLTO
#define MACHTHREAD_HPP_INMSKLTO

#include <mach/mach.h>
#include <memory>
#include <unistd.h>

namespace mad {
class MachThread {
  thread_act_t Id;

  x86_thread_state_t thread_state = {};
  mach_msg_type_number_t thread_state_count = x86_THREAD_STATE_COUNT;

public:
  MachThread(thread_act_t Id) : Id(Id){};
  MachThread(const MachThread &) = delete;
  MachThread operator=(const MachThread &) = delete;
  MachThread(MachThread &&) = default;
  MachThread &operator=(MachThread &&) = default;

  thread_act_t GetId() { return Id; }

  //FIXME: nedd a better prefix, maybe fetch?
  bool GetThreadState();
  bool SetThreadState();

  //FIXME: nedd a better prefix, maybe fetch?
  bool GetStates();
  bool SetStates();

  x86_thread_state64_t *ThreadState64() { return &thread_state.uts.ts64; }
  x86_thread_state32_t *ThreadState32() { return &thread_state.uts.ts32; }
};
} // namespace mad

#endif /* end of include guard: MACHTHREAD_HPP_INMSKLTO */
