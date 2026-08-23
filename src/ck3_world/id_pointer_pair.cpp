#include "id_pointer_pair.hpp"

#include <memory>
//
// template <typename T>
// ck3::IdPointerPair<T>::IdPointerPair(long long id)
//{
//}

template <typename T>
void ck3::IdPointerPair<T>::SetPointer(std::weak_ptr<T> new_pointer)
{
   pointer_ = new_pointer;
}