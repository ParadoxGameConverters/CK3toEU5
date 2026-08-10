#ifndef CK3_PAIR_H
#define CK3_PAIR_H

#include <memory>

namespace ck3
{
template <class T>
class IdPointerPair
{
  public:
   IdPointerPair() = default;
   explicit IdPointerPair(long long id): id_(id), pointer_(nullptr) {}
   [[nodiscard]] long long GetID() { return id_; }
   [[nodiscard]] std::shared_ptr<T> GetPointer() { return pointer_; }

   void SetPointer(std::shared_ptr<T> new_pointer);

  private:
   long long id_ = -1;
   std::shared_ptr<T> pointer_;
};
}  // namespace ck3

#endif  // CK3_PAIR_H
