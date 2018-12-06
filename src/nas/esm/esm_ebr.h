#define ESM_EBI_UNASSIGNED  (EPS_BEARER_IDENTITY_UNASSIGNED)
int esm_ebr_release (esm_context_t * esm_context, bearer_context_t * bearer_context, pdn_context_t * pdn_context, bool ue_requested);
int esm_ebr_start_timer(esm_context_t * esm_context, ebi_t ebi, CLONE_REF const_bstring msg, long sec, nas_timer_callback_t cb);
int esm_ebr_stop_timer(esm_context_t * esm_context, ebi_t ebi);
