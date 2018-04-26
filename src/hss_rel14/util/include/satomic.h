/*
* Copyright (c) 2017 Sprint
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __SATOMIC_H
#define __SATOMIC_H

#include <ext/atomicity.h>
#define atomic_dec(a) __sync_sub_and_fetch((unsigned long *)&a,1)
#define atomic_inc(a) __sync_add_and_fetch((unsigned long *)&a,1)
#define atomic_cas(a,b,c) __sync_val_compare_and_swap((unsigned long *)&a,b,c)
#define atomic_swap(a,b) __sync_lock_test_and_set((unsigned long *)&a,b);
#define atomic_and(a,b) __sync_fetch_and_and((unsigned long *)&a,b)
#define atomic_or(a,b) __sync_fetch_and_or((unsigned long *)&a,b)

#endif // #define __SATOMIC_H

