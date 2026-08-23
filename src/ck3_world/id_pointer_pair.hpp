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
   explicit IdPointerPair(long long the_id): id_(the_id), pointer_() {}
   [[nodiscard]] long long GetID() const { return id_; }
   [[nodiscard]] std::weak_ptr<T> GetPointer() const { return pointer_; }

   void SetPointer(std::weak_ptr<T> new_pointer);

  private:
   long long id_ = -1;
   std::weak_ptr<T> pointer_;
};
}  // namespace ck3

#endif  // CK3_PAIR_H
