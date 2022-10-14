//
// Created by Morvan on 2022/10/8.
//


#include "btorSim.h"

namespace wamcer::sim {
#define BTOR2_NEWN(ptr, nelems) \
  ((ptr) = (decltype(ptr)) btorsim_malloc ((nelems) * sizeof *(ptr)))

#define BTOR2_CNEWN(ptr, nelems) \
  ((ptr) = (decltype(ptr)) btorsim_calloc ((nelems), sizeof *(ptr)))

#define BTOR2_CLRN(ptr, nelems) (memset ((ptr), 0, (nelems) * sizeof *(ptr)))

#define BTOR2_REALLOC(p, n) \
  ((p) = (decltype(p)) btorsim_realloc ((p), ((n) * sizeof *(p))))

    BTOR2_DECLARE_STACK (Btor2LinePtr, Btor2Line *);
    BTOR2_DECLARE_STACK (BtorLong, int64_t);

    class BtorSim {
    public:
        BtorSim(std::string filename, SmtSolver solver) :
                slv(solver), model_path(filename) {
            if (!(model_file = fopen(model_path.c_str(), "r"))) {
                logger.err(0, "failed to open model file");
            }
            parse();
        }

        ~BtorSim() {
            fclose(model_file);
            for (int64_t i = 0; i < num_format_lines; i++)
                if (current_state[i]) btorsim_bv_free(current_state[i]);
            for (int64_t i = 0; i < num_format_lines; i++)
                if (next_state[i]) btorsim_bv_free(next_state[i]);
            BTOR2_RELEASE_STACK (inputs);
            BTOR2_RELEASE_STACK (states);
            BTOR2_RELEASE_STACK (bads);
            BTOR2_RELEASE_STACK (justices);
            BTOR2_RELEASE_STACK (reached_bads);
            BTOR2_RELEASE_STACK (constraints);
            btor2parser_delete(model);
            BTOR2_DELETE (inits);
            BTOR2_DELETE (nexts);
            BTOR2_DELETE (current_state);
            BTOR2_DELETE (next_state);
        }

        Term run(int seed, int step) {
            BTOR2_CNEWN (current_state, num_format_lines);
            BTOR2_CNEWN (next_state, num_format_lines);
            btorsim_rng_init(&rng, (uint32_t) seed);
            random_simulation(step);
            return Term();
        }

    private:

        SmtSolver slv;
        FILE *model_file;

        std::string model_path;
        Btor2LinePtrStack inputs;
        Btor2LinePtrStack states;
        Btor2LinePtrStack bads;
        Btor2LinePtrStack constraints;
        Btor2LinePtrStack justices;
        BtorLongStack reached_bads;
        Btor2Parser *model;
        int64_t num_format_lines;
        Btor2Line **inits;
        Btor2Line **nexts;
        int64_t num_unreached_bads;
        BtorSimBitVector **current_state;
        BtorSimBitVector **next_state;

        BtorSimRNG rng;

        void parse() {
            BTOR2_INIT_STACK (inputs);
            BTOR2_INIT_STACK (states);
            BTOR2_INIT_STACK (bads);
            BTOR2_INIT_STACK (justices);
            BTOR2_INIT_STACK (reached_bads);
            BTOR2_INIT_STACK (constraints);
            assert (model_file);
            model = btor2parser_new();
            if (!btor2parser_read_lines(model, model_file)) {
                logger.err(0, "parse error");
            }
            num_format_lines = btor2parser_max_id(model);
            BTOR2_CNEWN (inits, num_format_lines);
            BTOR2_CNEWN (nexts, num_format_lines);
            BTOR2_CNEWN (nexts, num_format_lines);
            Btor2LineIterator it = btor2parser_iter_init(model);
            Btor2Line *line;
            while ((line = btor2parser_iter_next(&it))) parse_model_line(line);
        }

        void parse_model_line(Btor2Line *l) {
            switch (l->tag) {
                case BTOR2_TAG_bad: {
                    int64_t i = (int64_t) BTOR2_COUNT_STACK (bads);
                    logger.log(defines::logSim, 2, "bad {} at line {}", i, l->lineno);
                    BTOR2_PUSH_STACK (bads, l);
                    BTOR2_PUSH_STACK (reached_bads, -1);
                    num_unreached_bads++;
                }
                    break;

                case BTOR2_TAG_constraint: {
                    int64_t i = (int64_t) BTOR2_COUNT_STACK (constraints);
                    logger.log(defines::logSim, 2, "constraint {} at line {}", i, l->lineno);
                    BTOR2_PUSH_STACK (constraints, l);
                }
                    break;

                case BTOR2_TAG_init:
                    inits[l->args[0]] = l;
                    break;

                case BTOR2_TAG_input: {
                    int64_t i = (int64_t) BTOR2_COUNT_STACK (inputs);
                    if (l->symbol) {
                        logger.log(defines::logSim, 2, "input {} {} at line {}", i, l->symbol, l->lineno);
                    } else {
                        logger.log(defines::logSim, 2, "input {} at line {}", i, l->lineno);
                    }
                    BTOR2_PUSH_STACK (inputs, l);
                }
                    break;

                case BTOR2_TAG_next:
                    nexts[l->args[0]] = l;
                    break;

                case BTOR2_TAG_sort: {
                    switch (l->sort.tag) {
                        case BTOR2_TAG_SORT_bitvec:
                            logger.log(defines::logSim, 2, "sort {} bitvec {} at line {}", l->id, l->sort.bitvec.width,
                                       l->lineno);
                            break;
                        case BTOR2_TAG_SORT_array:
                        default:
                            logger.err(0, "parse error in {} at line {}: unsupported sort {}", model_path, l->lineno,
                                       l->sort.name);
                            break;
                    }
                }
                    break;

                case BTOR2_TAG_state: {
                    int64_t i = (int64_t) BTOR2_COUNT_STACK (states);
                    if (l->symbol) {
                        logger.log(defines::logSim, 2, "state {} {} at line {}", i, l->symbol, l->lineno);
                    } else {
                        logger.log(defines::logSim, 2, "state {} at line {}", i, l->lineno);
                    }
                    BTOR2_PUSH_STACK (states, l);
                }
                    break;

                case BTOR2_TAG_add:
                case BTOR2_TAG_and:
                case BTOR2_TAG_concat:
                case BTOR2_TAG_const:
                case BTOR2_TAG_constd:
                case BTOR2_TAG_consth:
                case BTOR2_TAG_eq:
                case BTOR2_TAG_implies:
                case BTOR2_TAG_ite:
                case BTOR2_TAG_mul:
                case BTOR2_TAG_nand:
                case BTOR2_TAG_neq:
                case BTOR2_TAG_nor:
                case BTOR2_TAG_not:
                case BTOR2_TAG_one:
                case BTOR2_TAG_ones:
                case BTOR2_TAG_or:
                case BTOR2_TAG_redand:
                case BTOR2_TAG_redor:
                case BTOR2_TAG_slice:
                case BTOR2_TAG_sub:
                case BTOR2_TAG_uext:
                case BTOR2_TAG_ugt:
                case BTOR2_TAG_ugte:
                case BTOR2_TAG_ult:
                case BTOR2_TAG_ulte:
                case BTOR2_TAG_xnor:
                case BTOR2_TAG_xor:
                case BTOR2_TAG_zero:
                    break;

                case BTOR2_TAG_dec:
                case BTOR2_TAG_fair:
                case BTOR2_TAG_iff:
                case BTOR2_TAG_inc:
                case BTOR2_TAG_justice:
                case BTOR2_TAG_neg:
                case BTOR2_TAG_output:
                case BTOR2_TAG_read:
                case BTOR2_TAG_redxor:
                case BTOR2_TAG_rol:
                case BTOR2_TAG_ror:
                case BTOR2_TAG_saddo:
                case BTOR2_TAG_sdiv:
                case BTOR2_TAG_sdivo:
                case BTOR2_TAG_sext:
                case BTOR2_TAG_sgt:
                case BTOR2_TAG_sgte:
                case BTOR2_TAG_sll:
                case BTOR2_TAG_slt:
                case BTOR2_TAG_slte:
                case BTOR2_TAG_smod:
                case BTOR2_TAG_smulo:
                case BTOR2_TAG_sra:
                case BTOR2_TAG_srem:
                case BTOR2_TAG_srl:
                case BTOR2_TAG_ssubo:
                case BTOR2_TAG_uaddo:
                case BTOR2_TAG_udiv:
                case BTOR2_TAG_umulo:
                case BTOR2_TAG_urem:
                case BTOR2_TAG_usubo:
                case BTOR2_TAG_write:
                default:
                    logger.err(0, "parse error in {} at line {}: unsupported tag {} {}{}", model_path, l->lineno, l->id,
                               l->name, l->nargs ? " ..." : "");
                    break;
            }
        }

        void random_simulation(int64_t k) {
            logger.log(defines::logSim, 2, "starting random simulation up to bound {}", k);
            assert (k >= 0);

            const int32_t randomize = 1;

            initialize_states(randomize);
            initialize_inputs(0, randomize);
            simulate_step(0, randomize);

            for (int64_t i = 1; i <= k; i++) {
                transition(i);
                initialize_inputs(i, randomize);
                simulate_step(i, randomize);
            }
        }

        void initialize_states(int32_t randomly) {
            logger.log(defines::logSim, 2, "initialize states randomly {}", randomly);
            logger.log(defines::logSim, 2, "#0");
            for (int64_t i = 0; i < BTOR2_COUNT_STACK (states); i++) {
                Btor2Line *state = BTOR2_PEEK_STACK (states, i);
                assert (0 <= state->id), assert (state->id < num_format_lines);
                if (current_state[state->id]) continue;
                Btor2Line *init = inits[state->id];
                BtorSimBitVector *update;
                if (init) {
                    assert (init->nargs == 2);
                    assert (init->args[0] == state->id);
                    update = simulate(init->args[1]);
                } else {
                    assert (state->sort.tag == BTOR2_TAG_SORT_bitvec);
                    uint32_t width = state->sort.bitvec.width;
                    if (randomly)
                        update = btorsim_bv_new_random(&rng, width);
                    else
                        update = btorsim_bv_new(width);
                }
                update_current_state(state->id, update);
                if (!init) {
                    btorsim_bv_print_without_new_line(update);
                    if (state->symbol) printf(" %s#0", state->symbol);
                    fputc('\n', stdout);
                }
            }
        }

        BtorSimBitVector *simulate(int64_t id) {
            int32_t sign = id < 0 ? -1 : 1;
            if (sign < 0) id = -id;
            assert (0 <= id), assert (id < num_format_lines);
            BtorSimBitVector *res = current_state[id];
            if (!res) {
                Btor2Line *l = btor2parser_get_line_by_id(model, id);
                if (!l) {
                    logger.err(0, "internal error: undefined line {}", id);
                }
                BtorSimBitVector *args[3] = {0, 0, 0};
                for (uint32_t i = 0; i < l->nargs; i++) args[i] = simulate(l->args[i]);
                switch (l->tag) {
                    case BTOR2_TAG_add:
                        assert (l->nargs == 2);
                        res = btorsim_bv_add(args[0], args[1]);
                        break;
                    case BTOR2_TAG_and:
                        assert (l->nargs == 2);
                        res = btorsim_bv_and(args[0], args[1]);
                        break;
                    case BTOR2_TAG_concat:
                        assert (l->nargs == 2);
                        res = btorsim_bv_concat(args[0], args[1]);
                        break;
                    case BTOR2_TAG_const:
                        assert (l->nargs == 0);
                        res = btorsim_bv_char_to_bv(l->constant);
                        break;
                    case BTOR2_TAG_constd:
                        assert (l->nargs == 0);
                        res = btorsim_bv_constd(l->constant, l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_consth:
                        assert (l->nargs == 0);
                        res = btorsim_bv_consth(l->constant, l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_eq:
                        assert (l->nargs == 2);
                        res = btorsim_bv_eq(args[0], args[1]);
                        break;
                    case BTOR2_TAG_implies:
                        assert (l->nargs == 2);
                        res = btorsim_bv_implies(args[0], args[1]);
                        break;
                    case BTOR2_TAG_ite:
                        assert (l->nargs == 3);
                        res = btorsim_bv_ite(args[0], args[1], args[2]);
                        break;
                    case BTOR2_TAG_mul:
                        assert (l->nargs == 2);
                        res = btorsim_bv_mul(args[0], args[1]);
                        break;
                    case BTOR2_TAG_nand:
                        assert (l->nargs == 2);
                        res = btorsim_bv_nand(args[0], args[1]);
                        break;
                    case BTOR2_TAG_neq:
                        assert (l->nargs == 2);
                        res = btorsim_bv_neq(args[0], args[1]);
                        break;
                    case BTOR2_TAG_nor:
                        assert (l->nargs == 2);
                        res = btorsim_bv_nor(args[0], args[1]);
                        break;
                    case BTOR2_TAG_not:
                        assert (l->nargs == 1);
                        res = btorsim_bv_not(args[0]);
                        break;
                    case BTOR2_TAG_one:
                        res = btorsim_bv_one(l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_ones:
                        res = btorsim_bv_ones(l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_or:
                        assert (l->nargs == 2);
                        res = btorsim_bv_or(args[0], args[1]);
                        break;
                    case BTOR2_TAG_redand:
                        assert (l->nargs == 1);
                        res = btorsim_bv_redand(args[0]);
                        break;
                    case BTOR2_TAG_redor:
                        assert (l->nargs == 1);
                        res = btorsim_bv_redor(args[0]);
                        break;
                    case BTOR2_TAG_slice:
                        assert (l->nargs == 1);
                        res = btorsim_bv_slice(args[0], l->args[1], l->args[2]);
                        break;
                    case BTOR2_TAG_sub:
                        assert (l->nargs == 2);
                        res = btorsim_bv_sub(args[0], args[1]);
                        break;
                        break;
                    case BTOR2_TAG_uext:
                        assert (l->nargs == 1);
                        {
                            uint32_t width = args[0]->width;
                            assert (width <= l->sort.bitvec.width);
                            uint32_t padding = l->sort.bitvec.width - width;
                            if (padding)
                                res = btorsim_bv_uext(args[0], padding);
                            else
                                res = btorsim_bv_copy(args[0]);
                        }
                        break;
                    case BTOR2_TAG_ugt:
                        assert (l->nargs == 2);
                        res = btorsim_bv_ult(args[1], args[0]);
                        break;
                    case BTOR2_TAG_ugte:
                        assert (l->nargs == 2);
                        res = btorsim_bv_ulte(args[1], args[0]);
                        break;
                    case BTOR2_TAG_ult:
                        assert (l->nargs == 2);
                        res = btorsim_bv_ult(args[0], args[1]);
                        break;
                    case BTOR2_TAG_ulte:
                        assert (l->nargs == 2);
                        res = btorsim_bv_ulte(args[0], args[1]);
                        break;
                    case BTOR2_TAG_xnor:
                        assert (l->nargs == 2);
                        res = btorsim_bv_xnor(args[0], args[1]);
                        break;
                    case BTOR2_TAG_xor:
                        assert (l->nargs == 2);
                        res = btorsim_bv_xor(args[0], args[1]);
                        break;
                    case BTOR2_TAG_zero:
                        res = btorsim_bv_zero (l->sort.bitvec.width);
                        break;
                    default:
                        logger.err(0, "can not randomly simulate operator {} at line {}", l->name, l->lineno);
                        break;
                }
                for (uint32_t i = 0; i < l->nargs; i++) btorsim_bv_free(args[i]);
                update_current_state(id, res);
            }
            res = btorsim_bv_copy(res);
            if (sign < 0) {
                BtorSimBitVector *tmp = btorsim_bv_not(res);
                btorsim_bv_free(res);
                res = tmp;
            }
            return res;
        }

        void update_current_state(int64_t id, BtorSimBitVector *bv) {
            assert (0 <= id), assert (id < num_format_lines);
            if (current_state[id]) btorsim_bv_free(current_state[id]);
            current_state[id] = bv;
        }

        void initialize_inputs(int64_t k, int32_t randomize) {
            logger.log(defines::logSim, 2, "initializing inputs @{}", k);
            for (int64_t i = 0; i < BTOR2_COUNT_STACK (inputs); i++) {
                Btor2Line *input = BTOR2_PEEK_STACK (inputs, i);
                uint32_t width = input->sort.bitvec.width;
                if (current_state[input->id]) continue;
                BtorSimBitVector *update;
                if (randomize)
                    update = btorsim_bv_new_random(&rng, width);
                else
                    update = btorsim_bv_new(width);
                update_current_state(input->id, update);
                logger.log(2, "{}", i);
                btorsim_bv_print_without_new_line(update);
                if (input->symbol) {
                    logger.log(2, " {}@{}", input->symbol, k);
                }
            }
        }

        void simulate_step(int64_t k, int32_t randomize_states_that_are_inputs) {
            logger.log(defines::logSim, 2, "simulating step {}", k);
            for (int64_t i = 0; i < num_format_lines; i++) {
                Btor2Line *l = btor2parser_get_line_by_id(model, i);
                if (!l) continue;
                if (l->tag == BTOR2_TAG_sort || l->tag == BTOR2_TAG_init
                    || l->tag == BTOR2_TAG_next || l->tag == BTOR2_TAG_bad
                    || l->tag == BTOR2_TAG_constraint || l->tag == BTOR2_TAG_fair
                    || l->tag == BTOR2_TAG_justice)
                    continue;

                BtorSimBitVector *bv = simulate(i);
                btorsim_bv_free(bv);
            }
            for (int64_t i = 0; i < BTOR2_COUNT_STACK (states); i++) {
                Btor2Line *state = BTOR2_PEEK_STACK (states, i);
                assert (0 <= state->id), assert (state->id < num_format_lines);
                Btor2Line *next = nexts[state->id];
                BtorSimBitVector *update;
                if (next) {
                    assert (next->nargs == 2);
                    assert (next->args[0] == state->id);
                    update = simulate(next->args[1]);
                } else {
                    assert (state->sort.tag == BTOR2_TAG_SORT_bitvec);
                    uint32_t width = state->sort.bitvec.width;
                    if (randomize_states_that_are_inputs)
                        update = btorsim_bv_new_random(&rng, width);
                    else
                        update = btorsim_bv_new(width);
                }
                assert (!next_state[state->id]);
                next_state[state->id] = update;
            }
        }

        void transition(int64_t k) {
            logger.log(defines::logSim, 2, "transition {}", k);
            for (int64_t i = 0; i < num_format_lines; i++) delete_current_state(i);
            logger.log(defines::logSim, 2, "#{}", k);
            for (int64_t i = 0; i < BTOR2_COUNT_STACK (states); i++) {
                Btor2Line *state = BTOR2_PEEK_STACK (states, i);
                assert (0 <= state->id), assert (state->id < num_format_lines);
                BtorSimBitVector *update = next_state[state->id];
                assert (update);
                update_current_state(state->id, update);
                next_state[state->id] = 0;
                logger.log(defines::logSim, " ", 2, "{}", i);
                btorsim_bv_print_without_new_line(update);
                std::cout << std::endl;
                if (state->symbol) {
                    logger.log(defines::logSim, 2, "{}#{}", state->symbol, k);
                }
            }
        }

        void delete_current_state(int64_t id) {
            assert (0 <= id), assert (id < num_format_lines);
            if (current_state[id]) btorsim_bv_free(current_state[id]);
            current_state[id] = 0;
        }
    };

    Term randomSim(std::string path, SmtSolver solver, int bound, int seed) {
        auto sim = BtorSim(path, solver);
        return sim.run(seed, bound);
    }
}