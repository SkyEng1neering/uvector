/**
 * @file uvector_test_conf.h
 * @brief dalloc configuration overrides for uvector unit tests
 */

#ifndef UVECTOR_TEST_CONF_H
#define UVECTOR_TEST_CONF_H

/* Disable debug output for cleaner test output */
#define dalloc_debug(...)
#define uvector_debug(...)

#endif /* UVECTOR_TEST_CONF_H */
