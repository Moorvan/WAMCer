//
// Created by Morvan on 2022/10/8.
//


#include "btorSim.h"

int32_t verbosity;
namespace wamcer::sim {

    class BtorSim {

    public:
        BtorSim(std::string filename, SmtSolver solver) :
                slv(solver), model_path(filename), ts(solver),
                btor2(filename, ts) {
            if (!(model_file = fopen((char *) model_path.data(), "r"))) {
                logger.err("failed to open btor2 model file '{}'", model_path);
            }
            parse_model();
            setup_states();
            fclose(model_file);
            states_slv = btor2.statesvec();
            concrete_states = TermVec();
            concrete_states_str = std::vector<std::string>();
            concrete_states_set = std::unordered_set < std::string > ();
        }

        ~BtorSim() {
            btor2parser_delete(model);
            for (int64_t i = 0; i < num_format_lines; i++)
                if (current_state[i].type) current_state[i].remove();
            for (int64_t i = 0; i < num_format_lines; i++)
                if (next_state[i].type) next_state[i].remove();
        }

        TermVec run(int seed, int step, std::string filepath) {
            btorsim_rng_init(&rng, (uint32_t) seed);
            random_simulation(step);
            if (filepath != "") {
                write_into_file(filepath);
            }
            return concrete_states;
        }

    private:
        Btor2Parser *model;

        SmtSolver slv;
        TransitionSystem ts;
        BTOR2Encoder btor2;
        TermVec states_slv;
        TermVec concrete_states;
        std::vector<std::string> concrete_states_str;
        std::unordered_set<std::string> concrete_states_set;

        std::vector<Btor2Line *> inputs;
        std::vector<Btor2Line *> states;
//    std::vector<Btor2Line *> bads;
//    std::vector<Btor2Line *> constraints;
//    std::vector<Btor2Line *> justices;

//    std::vector<int64_t> reached_bads;

//    int64_t constraints_violated = -1;
//    int64_t num_unreached_bads;

        std::map<int64_t, std::string> extra_constraints;

        int64_t num_format_lines;
        std::vector<Btor2Line *> inits;
        std::vector<Btor2Line *> nexts;

        std::vector<BtorSimState> current_state;
        std::vector<BtorSimState> next_state;
        FILE *model_file;
        std::string model_path;

        BtorSimRNG rng;

        void parse_model_line(Btor2Line *l) {
            switch (l->tag) {
                case BTOR2_TAG_bad:
//                int64_t i = (int64_t) bads.size();
//                logger.log(defines::logSim, 2, "bad {} at line {}", i, l->lineno);
//                bads.push_back(l);
//                reached_bads.push_back(-1);
//                num_unreached_bads++;
//            }
                    break;

                case BTOR2_TAG_constraint:
//                int64_t i = (int64_t) constraints.size();
//                logger.log(defines::logSim, 2, "constraint {} at line {}", i, l->lineno);
//                constraints.push_back(l);
//            }
                    break;

                case BTOR2_TAG_init:
                    inits[l->args[0]] = l;
                    break;

                case BTOR2_TAG_input: {
                    int64_t i = (int64_t) inputs.size();
                    if (l->symbol) {
                        logger.log(defines::logSim, 2, "input {} {} at line {}", i, l->symbol, l->lineno);
                    } else {
                        logger.log(defines::logSim, 2, "input {} at line {}", i, l->lineno);
                    }
                    inputs.push_back(l);
                }
                    break;

                case BTOR2_TAG_next:
                    nexts[l->args[0]] = l;
                    break;

                case BTOR2_TAG_sort: {
                    switch (l->sort.tag) {
                        case BTOR2_TAG_SORT_bitvec:
                            logger.log(defines::logSim, 2, "sort bitvec {} at line {}", l->sort.bitvec.width,
                                       l->lineno);
                            break;
                        case BTOR2_TAG_SORT_array:
                            logger.log(defines::logSim, 2, "sort array {} {} at line {}",
                                       l->sort.array.index, l->sort.array.element, l->lineno);
                            break;
                        default:
                            logger.err("parse error in {} at line {}: unsupported sort {}", model_path, l->lineno,
                                       l->sort.name);
                            break;
                    }
                }
                    break;

                case BTOR2_TAG_state: {
                    int64_t i = (int64_t) states.size();
                    if (l->symbol) {
                        logger.log(defines::logSim, 2, "state {} {} at line {}", i, l->symbol, l->lineno);
                    } else {
                        logger.log(defines::logSim, 2, "state {} at line {}", i, l->lineno);
                    }
                    states.push_back(l);
                }
                    break;

                case BTOR2_TAG_add:
                case BTOR2_TAG_and:
                case BTOR2_TAG_concat:
                case BTOR2_TAG_const:
                case BTOR2_TAG_constd:
                case BTOR2_TAG_consth:
                case BTOR2_TAG_dec:
                case BTOR2_TAG_eq:
                case BTOR2_TAG_implies:
                case BTOR2_TAG_inc:
                case BTOR2_TAG_ite:
                case BTOR2_TAG_mul:
                case BTOR2_TAG_nand:
                case BTOR2_TAG_neg:
                case BTOR2_TAG_neq:
                case BTOR2_TAG_nor:
                case BTOR2_TAG_not:
                case BTOR2_TAG_one:
                case BTOR2_TAG_ones:
                case BTOR2_TAG_or:
                case BTOR2_TAG_output:
                case BTOR2_TAG_redand:
                case BTOR2_TAG_redor:
                case BTOR2_TAG_redxor:
                case BTOR2_TAG_sdiv:
                case BTOR2_TAG_sext:
                case BTOR2_TAG_sgt:
                case BTOR2_TAG_sgte:
                case BTOR2_TAG_slice:
                case BTOR2_TAG_sll:
                case BTOR2_TAG_slt:
                case BTOR2_TAG_slte:
                case BTOR2_TAG_sra:
                case BTOR2_TAG_srem:
                case BTOR2_TAG_srl:
                case BTOR2_TAG_sub:
                case BTOR2_TAG_udiv:
                case BTOR2_TAG_uext:
                case BTOR2_TAG_ugt:
                case BTOR2_TAG_ugte:
                case BTOR2_TAG_ult:
                case BTOR2_TAG_ulte:
                case BTOR2_TAG_urem:
                case BTOR2_TAG_xnor:
                case BTOR2_TAG_xor:
                case BTOR2_TAG_zero:
                case BTOR2_TAG_read:
                case BTOR2_TAG_write:
                    break;

                case BTOR2_TAG_fair:
                case BTOR2_TAG_justice:
                case BTOR2_TAG_rol:
                case BTOR2_TAG_ror:
                case BTOR2_TAG_saddo:
                case BTOR2_TAG_sdivo:
                case BTOR2_TAG_smod:
                case BTOR2_TAG_smulo:
                case BTOR2_TAG_ssubo:
                case BTOR2_TAG_uaddo:
                case BTOR2_TAG_umulo:
                case BTOR2_TAG_usubo:
                default:
                    logger.err("parse error in {} at line {}: unsupported {} {}{}", model_path, l->lineno, l->id,
                               l->name,
                               l->nargs ? " ..." : "");
                    break;
            }
        }

        void parse_model() {
            assert (model_file);
            model = btor2parser_new();
            if (!btor2parser_read_lines(model, model_file)) {
                logger.err("parse error in '{}' at {}", model_path, btor2parser_error(model));
            }
            num_format_lines = btor2parser_max_id(model);
            inits.resize(num_format_lines, nullptr);
            nexts.resize(num_format_lines, nullptr);
            Btor2LineIterator it = btor2parser_iter_init(model);
            Btor2Line *line;
            while ((line = btor2parser_iter_next(&it))) parse_model_line(line);
            for (size_t i = 0; i < states.size(); i++) {
                Btor2Line *state = states[i];
                if (!nexts[state->id]) {
                    logger.err("state {} has no next state", state->id);
                }
            }
        }

        void setup_states() {
            current_state.resize(num_format_lines);
            next_state.resize(num_format_lines);

            for (int i = 0; i < num_format_lines; i++) {
                Btor2Line *l = btor2parser_get_line_by_id(model, i);
                if (l) {
                    Btor2Sort *sort = get_sort(l, model);
                    switch (sort->tag) {
                        case BTOR2_TAG_SORT_bitvec:
                            current_state[i].type = BtorSimState::Type::BITVEC;
                            next_state[i].type = BtorSimState::Type::BITVEC;
                            break;
                        case BTOR2_TAG_SORT_array:
                            current_state[i].type = BtorSimState::Type::ARRAY;
                            next_state[i].type = BtorSimState::Type::ARRAY;
                            break;
                        default:
                            logger.err("Unknown sort");
                    }
                }
            }

            for (auto state: states) {
                assert (current_state[state->id].type != BtorSimState::Type::INVALID);
                assert (next_state[state->id].type != BtorSimState::Type::INVALID);
            }
        }

        void update_current_state(int64_t id, BtorSimState &s) {
            assert (0 <= id), assert (id < num_format_lines);
            logger.log(defines::logSim, 2, "update_current_state {}", id);
            current_state[id].update(s);
        }

        void update_current_state(int64_t id, BtorSimArrayModel *am) {
            assert (0 <= id), assert (id < num_format_lines);
            logger.log(defines::logSim, 2, "update_current_state {}", id);
            current_state[id].update(am);
        }

        void update_current_state(int64_t id, BtorSimBitVector *bv) {
            assert (0 <= id), assert (id < num_format_lines);
            logger.log(defines::logSim, 2, "update_current_state {}", id);
            current_state[id].update(bv);
        }

        void delete_current_state(int64_t id) {
            assert (0 <= id), assert (id < num_format_lines);
            if (current_state[id].type) current_state[id].remove();
        }

        BtorSimState simulate(int64_t id) {
            int32_t sign = id < 0 ? -1 : 1;
            if (sign < 0) id = -id;
            assert (0 <= id), assert (id < num_format_lines);
            BtorSimState res = current_state[id];
            if (!res.is_set()) {
                Btor2Line *l = btor2parser_get_line_by_id(model, id);
                if (!l) {
                    logger.err("internal error: unexpected empty ID {}", id);
                }
                BtorSimState args[3];
                for (uint32_t i = 0; i < l->nargs; i++) args[i] = simulate(l->args[i]);
                switch (l->tag) {
                    case BTOR2_TAG_add:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_add(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_and:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_and(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_concat:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_concat(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_const:
                        assert (l->nargs == 0);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_char_to_bv(l->constant);
                        break;
                    case BTOR2_TAG_constd:
                        assert (l->nargs == 0);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_constd(l->constant, l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_consth:
                        assert (l->nargs == 0);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_consth(l->constant, l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_dec:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_dec(args[0].bv_state);
                        break;
                    case BTOR2_TAG_eq:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        if (args[0].type == BtorSimState::Type::ARRAY) {
                            assert (args[1].type == BtorSimState::Type::ARRAY);
                            res.bv_state =
                                    btorsim_am_eq(args[0].array_state, args[1].array_state);
                        } else {
                            assert (args[0].type == BtorSimState::Type::BITVEC);
                            assert (args[1].type == BtorSimState::Type::BITVEC);
                            res.bv_state = btorsim_bv_eq(args[0].bv_state, args[1].bv_state);
                        }
                        break;
                    case BTOR2_TAG_implies:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_implies(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_inc:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_inc(args[0].bv_state);
                        break;
                    case BTOR2_TAG_ite:
                        assert (l->nargs == 3);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        if (res.type == BtorSimState::Type::ARRAY) {
                            assert (args[1].type == BtorSimState::Type::ARRAY);
                            assert (args[2].type == BtorSimState::Type::ARRAY);
                            res.array_state = btorsim_am_ite(
                                    args[0].bv_state, args[1].array_state, args[2].array_state);
                        } else {
                            assert (args[1].type == BtorSimState::Type::BITVEC);
                            assert (args[2].type == BtorSimState::Type::BITVEC);
                            res.bv_state = btorsim_bv_ite(
                                    args[0].bv_state, args[1].bv_state, args[2].bv_state);
                        }
                        break;
                    case BTOR2_TAG_mul:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_mul(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_nand:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_nand(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_neg:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_neg(args[0].bv_state);
                        break;
                    case BTOR2_TAG_neq:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        if (args[0].type == BtorSimState::Type::ARRAY) {
                            assert (args[1].type == BtorSimState::Type::ARRAY);
                            res.bv_state =
                                    btorsim_am_neq(args[0].array_state, args[1].array_state);
                        } else {
                            assert (args[0].type == BtorSimState::Type::BITVEC);
                            assert (args[1].type == BtorSimState::Type::BITVEC);
                            res.bv_state = btorsim_bv_neq(args[0].bv_state, args[1].bv_state);
                        }
                        break;
                    case BTOR2_TAG_nor:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_nor(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_not:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_not(args[0].bv_state);
                        break;
                    case BTOR2_TAG_one:
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_one(l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_ones:
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_ones(l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_or:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_or(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_redand:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_redand(args[0].bv_state);
                        break;
                    case BTOR2_TAG_redor:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_redor(args[0].bv_state);
                        break;
                    case BTOR2_TAG_redxor:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_redxor(args[0].bv_state);
                        break;
                    case BTOR2_TAG_slice:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        res.bv_state =
                                btorsim_bv_slice(args[0].bv_state, l->args[1], l->args[2]);
                        break;
                    case BTOR2_TAG_sub:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_sub(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_uext:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        {
                            uint32_t width = args[0].bv_state->width;
                            assert (width <= l->sort.bitvec.width);
                            uint32_t padding = l->sort.bitvec.width - width;
                            if (padding)
                                res.bv_state = btorsim_bv_uext(args[0].bv_state, padding);
                            else
                                res.bv_state = btorsim_bv_copy(args[0].bv_state);
                        }
                        break;
                    case BTOR2_TAG_udiv:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_udiv(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_sdiv:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_sdiv(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_sext:
                        assert (l->nargs == 1);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        {
                            uint32_t width = args[0].bv_state->width;
                            assert (width <= l->sort.bitvec.width);
                            uint32_t padding = l->sort.bitvec.width - width;
                            if (padding)
                                res.bv_state = btorsim_bv_sext(args[0].bv_state, padding);
                            else
                                res.bv_state = btorsim_bv_copy(args[0].bv_state);
                        }
                        break;
                    case BTOR2_TAG_sll:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_sll(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_srl:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_srl(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_sra:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_sra(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_srem:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_srem(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_ugt:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_ult(args[1].bv_state, args[0].bv_state);
                        break;
                    case BTOR2_TAG_ugte:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_ulte(args[1].bv_state, args[0].bv_state);
                        break;
                    case BTOR2_TAG_ult:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC),
                                assert (args[0].type == BtorSimState::Type::BITVEC),
                                assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_ult(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_ulte:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_ulte(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_urem:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_urem(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_sgt:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_slt(args[1].bv_state, args[0].bv_state);
                        break;
                    case BTOR2_TAG_sgte:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_slte(args[1].bv_state, args[0].bv_state);
                        break;
                    case BTOR2_TAG_slt:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_slt(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_slte:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_slte(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_iff:
                    case BTOR2_TAG_xnor:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_xnor(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_xor:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::BITVEC);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_xor(args[0].bv_state, args[1].bv_state);
                        break;
                    case BTOR2_TAG_zero:
                        assert (res.type == BtorSimState::Type::BITVEC);
                        res.bv_state = btorsim_bv_zero (l->sort.bitvec.width);
                        break;
                    case BTOR2_TAG_read:
                        assert (l->nargs == 2);
                        assert (res.type == BtorSimState::Type::BITVEC);
                        assert (args[0].type == BtorSimState::Type::ARRAY);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        res.bv_state = args[0].array_state->read(args[1].bv_state);
                        {
                            Btor2Line *mem = btor2parser_get_line_by_id(model, l->args[0]);
                            logger.log(defines::logSim, 2, "read {}[{}] -> {}",
                                       mem->symbol ? mem->symbol : std::to_string(mem->id),
                                       btorsim_bv_to_string(args[1].bv_state), btorsim_bv_to_string(res.bv_state));
                        }
                        break;
                    case BTOR2_TAG_write:
                        assert (l->nargs == 3);
                        assert (res.type == BtorSimState::Type::ARRAY);
                        assert (args[0].type == BtorSimState::Type::ARRAY);
                        assert (args[1].type == BtorSimState::Type::BITVEC);
                        assert (args[2].type == BtorSimState::Type::BITVEC);
                        res.array_state =
                                args[0].array_state->write(args[1].bv_state, args[2].bv_state);
                        {
                            Btor2Line *mem = btor2parser_get_line_by_id(model, l->args[0]);
                            logger.log(defines::logSim, 2, "write {}[{}] -> {}",
                                       mem->symbol ? mem->symbol : std::to_string(mem->id),
                                       btorsim_bv_to_string(args[1].bv_state), btorsim_bv_to_string(args[2].bv_state));
                        }
                        break;
                    default:
                        logger.err("can not randomly simulate operator '{}' at line {}", l->name, l->lineno);
                        break;
                }
                for (uint32_t i = 0; i < l->nargs; i++) args[i].remove();
                update_current_state(id, res);
            }
            if (res.type == BtorSimState::Type::ARRAY) {
                res.array_state = res.array_state->copy();
            } else {
                assert (res.type == BtorSimState::Type::BITVEC);
                if (sign < 0)
                    res.bv_state = btorsim_bv_not(res.bv_state);
                else
                    res.bv_state = btorsim_bv_copy(res.bv_state);
            }
            return res;
        }

        void initialize_states(int32_t randomly) {
            logger.log(defines::logSim, 1, "initializing states at #0");

            auto cur = TermVec();
            auto curs = std::vector<std::string>();
            for (size_t i = 0; i < states.size(); i++) {
                Btor2Line *state = states[i];
                assert (0 <= state->id), assert (state->id < num_format_lines);
                Btor2Line *init = inits[state->id];
                if (!current_state[state->id].is_set()) {  // can be set in parse_state_part from witness
                    switch (current_state[state->id].type) {
                        case BtorSimState::Type::BITVEC: {
                            assert (state->sort.tag == BTOR2_TAG_SORT_bitvec);
                            if (init) {
                                assert (init->nargs == 2);
                                assert (init->args[0] == state->id);
                                BtorSimState update = simulate(init->args[1]);
                                assert (update.type == BtorSimState::Type::BITVEC);
                                auto bv = update.bv_state;
                                auto value = slv->make_term(btorsim_bv_to_uint64(bv), states_slv[i]->get_sort());
                                auto eq = slv->make_term(Equal, states_slv[i], value);
                                cur.push_back(eq);
                                curs.push_back(value->to_string());
                                update_current_state(state->id, update);
                            } else {
                                BtorSimBitVector *bv;
                                if (randomly)
                                    bv = btorsim_bv_new_random(&rng, state->sort.bitvec.width);
                                else
                                    bv = btorsim_bv_new(state->sort.bitvec.width);
                                auto value = slv->make_term(btorsim_bv_to_uint64(bv), states_slv[i]->get_sort());
                                auto eq = slv->make_term(Equal, states_slv[i], value);
                                cur.push_back(eq);
                                curs.push_back(value->to_string());
                                update_current_state(state->id, bv);
                            }
                        }
                            break;
                        case BtorSimState::Type::ARRAY:
                            assert (state->sort.tag == BTOR2_TAG_SORT_array);
                            if (init) {
                                assert (init->nargs == 2);
                                assert (init->args[0] == state->id);
                                BtorSimState update = simulate(init->args[1]);
                                BtorSimArrayModel *am;
                                auto kvs = std::vector<std::string>();
                                auto array_terms = std::vector<Term>();
                                auto key_sort = Sort();
                                auto value_sort = Sort();
                                switch (update.type) {
                                    case BtorSimState::Type::ARRAY:
                                        update_current_state(state->id, update);

                                        am = update.array_state;
                                        key_sort = slv->make_sort(BV, am->index_width);
                                        value_sort = slv->make_sort(BV, am->element_width);

                                        for (auto d: am->data) {
                                            auto key = slv->make_term(d.first, key_sort, 2);
                                            auto value = slv->make_term((int64_t) btorsim_bv_to_uint64(d.second),
                                                                        value_sort);
                                            auto v = slv->make_term(Select, states_slv[i], key);
                                            array_terms.push_back(slv->make_term(Equal, v, value));
                                            kvs.push_back(
                                                    d.first + ":" + std::to_string(btorsim_bv_to_uint64(d.second)));
                                        }
                                        curs.push_back(string_join(kvs, ";"));
                                        if (!array_terms.empty()) {
                                            if (array_terms.size() == 1) {
                                                cur.push_back(array_terms[0]);
                                            } else {
                                                cur.push_back(slv->make_term(And, array_terms));
                                            }
                                        }
                                        break;
                                    case BtorSimState::Type::BITVEC: {
                                        Btor2Line *li =
                                                btor2parser_get_line_by_id(model, state->sort.array.index);
                                        Btor2Line *le = btor2parser_get_line_by_id(
                                                model, state->sort.array.element);
                                        assert (li->sort.tag == BTOR2_TAG_SORT_bitvec);
                                        assert (le->sort.tag == BTOR2_TAG_SORT_bitvec);
                                        BtorSimArrayModel *am = new BtorSimArrayModel(
                                                li->sort.bitvec.width, le->sort.bitvec.width);
                                        am->const_init = update.bv_state;

                                        curs.push_back(btorsim_bv_to_string(am->const_init));
                                        auto key = slv->make_symbol("key" + std::to_string(time(0)), key_sort);
                                        auto value = slv->make_term((int64_t) btorsim_bv_to_uint64(am->const_init), value_sort);
                                        auto v = slv->make_term(Select, states_slv[i], key);
                                        cur.push_back(slv->make_term(Equal, v, value));

                                        update_current_state(state->id, am);
                                    }
                                        break;
                                    default:
                                        logger.err("bad result simulating {}", init->args[1]);
                                }
                            } else {
                                Btor2Line *li =
                                        btor2parser_get_line_by_id(model, state->sort.array.index);
                                Btor2Line *le =
                                        btor2parser_get_line_by_id(model, state->sort.array.element);
                                assert (li->sort.tag == BTOR2_TAG_SORT_bitvec);
                                assert (le->sort.tag == BTOR2_TAG_SORT_bitvec);
                                BtorSimArrayModel *am = new BtorSimArrayModel(
                                        li->sort.bitvec.width, le->sort.bitvec.width);
                                if (randomly) {
                                    am->random_seed = btorsim_rng_rand(&rng);
                                }
                                update_current_state(state->id, am);
                            }
                            break;
                        default:
                            logger.err("uninitialized current_state {}", state->id);
                    }
                }
//            if (print_trace && !init) print_state_or_input(state->id, i, 0, false);
            }
            auto state = string_join(curs, ",");
            if (concrete_states_set.find(state) == concrete_states_set.end()) {
                concrete_states_set.insert(state);
                concrete_states_str.push_back(state);
                if (cur.size() == 1) {
                    concrete_states.push_back(cur[0]);
                } else if (cur.size() > 1) {
                    concrete_states.push_back(slv->make_term(And, cur));
                }
            }
        }

        void initialize_inputs(int64_t k, int32_t randomize) {
            logger.log(defines::logSim, 1, "initializing inputs at @{}", k);
//        if (print_trace) printf("@%"
//        PRId64
//        "\n", k);
            for (size_t i = 0; i < inputs.size(); i++) {
                Btor2Line *input = inputs[i];
                if (!current_state[input->id].is_set())
                    // if not set previously by parse_input_part from witness
                {
                    if (input->sort.tag == BTOR2_TAG_SORT_bitvec) {
                        uint32_t width = input->sort.bitvec.width;
                        BtorSimBitVector *update;
                        if (randomize)
                            update = btorsim_bv_new_random(&rng, width);
                        else
                            update = btorsim_bv_new(width);
                        update_current_state(input->id, update);
                    } else {
                        assert (input->sort.tag == BTOR2_TAG_SORT_array);
                        Btor2Line *li =
                                btor2parser_get_line_by_id(model, input->sort.array.index);
                        Btor2Line *le =
                                btor2parser_get_line_by_id(model, input->sort.array.element);
                        assert (li->sort.tag == BTOR2_TAG_SORT_bitvec);
                        assert (le->sort.tag == BTOR2_TAG_SORT_bitvec);
                        BtorSimArrayModel *am = new BtorSimArrayModel(li->sort.bitvec.width,
                                                                      le->sort.bitvec.width);
                        if (randomize) {
                            am->random_seed = btorsim_rng_rand(&rng);
                        }
                        update_current_state(input->id, am);
                    }
                }
//            if (print_trace) print_state_or_input(input->id, i, k, true);
            }
        }

        void simulate_step(int64_t k, int32_t randomize_states_that_are_inputs) {
            logger.log(defines::logSim, 1, "simulating step {}", k);
            for (int64_t i = 0; i < num_format_lines; i++) {
                Btor2Line *l = btor2parser_get_line_by_id(model, i);
                if (!l) continue;
                if (l->tag == BTOR2_TAG_sort || l->tag == BTOR2_TAG_init
                    || l->tag == BTOR2_TAG_next || l->tag == BTOR2_TAG_bad
                    || l->tag == BTOR2_TAG_constraint || l->tag == BTOR2_TAG_fair
                    || l->tag == BTOR2_TAG_justice || l->tag == BTOR2_TAG_output)
                    continue;

                BtorSimState s = simulate(i);
                s.remove();
            }
            for (size_t i = 0; i < states.size(); i++) {
                Btor2Line *state = states[i];
                assert (0 <= state->id), assert (state->id < num_format_lines);
                Btor2Line *next = nexts[state->id];
                BtorSimState update;
                if (next) {
                    assert (next->nargs == 2);
                    assert (next->args[0] == state->id);
                    update = simulate(next->args[1]);
                } else {
                    if (state->sort.tag == BTOR2_TAG_SORT_bitvec) {
                        update.type = BtorSimState::Type::BITVEC;
                        uint32_t width = state->sort.bitvec.width;
                        if (randomize_states_that_are_inputs)
                            update.bv_state = btorsim_bv_new_random(&rng, width);
                        else
                            update.bv_state = btorsim_bv_new(width);
                    } else {
                        assert (state->sort.tag == BTOR2_TAG_SORT_array);
                        update.type = BtorSimState::Type::ARRAY;
                        Btor2Line *li =
                                btor2parser_get_line_by_id(model, state->sort.array.index);
                        Btor2Line *le =
                                btor2parser_get_line_by_id(model, state->sort.array.element);
                        assert (li->sort.tag == BTOR2_TAG_SORT_bitvec);
                        assert (le->sort.tag == BTOR2_TAG_SORT_bitvec);
                        update.array_state = new BtorSimArrayModel(li->sort.bitvec.width,
                                                                   le->sort.bitvec.width);
                        if (randomize_states_that_are_inputs)
                            update.array_state->random_seed = btorsim_rng_rand(&rng);
                    }
                }
                assert (!next_state[state->id].is_set());
                assert (next_state[state->id].type == update.type);
                next_state[state->id] = update;
            }
        }

        void transition(int64_t k) {
            logger.log(defines::logSim, 1, "transition {}", k);
            for (int64_t i = 0; i < num_format_lines; i++) delete_current_state(i);
            auto cur = TermVec();
            auto curs = std::vector<std::string>();
            for (size_t i = 0; i < states.size(); i++) {
                Btor2Line *state = states[i];
                assert (0 <= state->id), assert (state->id < num_format_lines);
                BtorSimState update = next_state[state->id];
                assert (update.is_set());
                auto value = Term();
                auto eq = Term();
                BtorSimBitVector *bv;
                BtorSimArrayModel *am;
                auto kvs = std::vector<std::string>();
                auto array_terms = std::vector<Term>();
                auto key_sort = Sort();
                auto value_sort = Sort();
                switch (update.type) {
                    case BtorSimState::Type::BITVEC:
                        bv = update.bv_state;
                        value = slv->make_term(btorsim_bv_to_uint64(bv), states_slv[i]->get_sort());
                        eq = slv->make_term(Equal, states_slv[i], value);
                        cur.push_back(eq);
                        curs.push_back(value->to_string());
                        break;
                    case BtorSimState::Type::ARRAY:
                        am = update.array_state;
                        key_sort = slv->make_sort(BV, am->index_width);
                        value_sort = slv->make_sort(BV, am->element_width);
                        for (auto d: am->data) {
                            auto key = slv->make_term(d.first, key_sort, 2);
                            auto value = slv->make_term((int64_t) btorsim_bv_to_uint64(d.second),
                                                        value_sort);
                            auto v = slv->make_term(Select, states_slv[i], key);
                            array_terms.push_back(slv->make_term(Equal, v, value));
                            kvs.push_back(d.first + ":" + std::to_string(btorsim_bv_to_uint64(d.second)));
                        }
                        curs.push_back(string_join(kvs, ";"));
                        if (!array_terms.empty()) {
                            if (array_terms.size() == 1) {
                                cur.push_back(array_terms[0]);
                            } else {
                                cur.push_back(slv->make_term(And, array_terms));
                            }
                        }
                        break;
                    default:
                        break;
                }
                update_current_state(state->id, update);
                switch (next_state[state->id].type) {
                    case BtorSimState::Type::BITVEC:
                        next_state[state->id].bv_state = nullptr;
                        break;
                    case BtorSimState::Type::ARRAY:
                        next_state[state->id].array_state = nullptr;
                        break;
                    default:
                        logger.err("Invalid state type");
                }
//            if (print_trace && print_states)
//                print_state_or_input (state->id, i, k, false);
            }
            auto state = string_join(curs, ",");
            if (concrete_states_set.find(state) == concrete_states_set.end()) {
                concrete_states_set.insert(state);
                concrete_states_str.push_back(state);
                if (cur.size() == 1) {
                    concrete_states.push_back(cur[0]);
                } else if (cur.size() > 1) {
                    concrete_states.push_back(slv->make_term(And, cur));
                }
            }
        }

        void random_simulation(int64_t k) {
            logger.log(defines::logSim, 1, "starting random simulation up to bound {}", k);
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

        void write_into_file(const std::string &filename) {
            auto file = std::ofstream(filename);
            if (!file.is_open()) {
                logger.log(defines::logSim, 0, "could not open file {}", filename);
            }
            auto names = std::vector<std::string>();
            auto sorts = std::vector<std::string>();
            std::for_each(states_slv.begin(), states_slv.end(), [&](auto &i) {
                names.push_back(i->to_string());
                sorts.push_back(i->get_sort()->to_string());
            });
            file << "name," << string_join(names, ",") << std::endl;
            file << "sort," << string_join(sorts, ",") << std::endl;
            for (auto i = 0; i < concrete_states_str.size(); i++) {
                file << i << "," << concrete_states_str[i] << std::endl;
            }
            file.close();
        }

        static std::string string_join(const std::vector<std::string> &v, const std::string &c) {
            auto ss = std::stringstream();
            for (uint32_t i = 0; i < v.size(); i++) {
                if (i != 0) ss << c;
                ss << v[i];
            }
            return ss.str();
        }
    };

    TermVec randomSim(std::string path, SmtSolver solver, std::string filepath, int bound, int seed) {
        auto sim = BtorSim(path, solver);
        return sim.run(seed, bound, filepath);
    }
}