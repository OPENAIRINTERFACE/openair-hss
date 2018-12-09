#define ESM_EBI_UNASSIGNED  (EPS_BEARER_IDENTITY_UNASSIGNED)
int esm_ebr_start_timer(esm_context_t * esm_context, ebi_t ebi, CLONE_REF const_bstring msg, long sec, nas_timer_callback_t cb);
