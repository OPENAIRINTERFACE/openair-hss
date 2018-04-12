find_package(Check REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CHECK_INCLUDE_DIRS})

set(MME_APP_UE_CONTEXT_IMSI_SRC   test_mme_app_ue_context.c)
add_executable(test_mme_app_ue_context_imsi ${MME_APP_UE_CONTEXT_IMSI_SRC})
target_link_libraries(test_mme_app_ue_context_imsi MME_APP ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


#set(TEST_AES_CMAC_SRC test_aes128_cmac_encrypt.c)
#add_executable(test_aes128_cmac ${TEST_AES_CMAC_SRC})
#target_link_libraries(test_aes128_cmac crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


#set(TEST_AES128_DECRYPT_SRC test_aes128_ctr_decrypt.c )
#add_executable(test_aes128_ctr_decrypt ${TEST_AES128_DECRYPT_SRC})
#target_link_libraries(test_aes128_ctr_decrypt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


#set(TEST_AES128_ENCRYPT_SRC test_aes128_ctr_encrypt.c )
#add_executable(test_aes128_ctr_encrypt ${TEST_AES128_ENCRYPT_SRC})
#target_link_libraries(test_aes128_ctr_encrypt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
