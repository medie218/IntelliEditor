/**
 * @file rule_parser.c
 * @brief Parser JSON des fichiers de règles — ADAPTER / rules_json_cjson
 */

#include "../../../include/rules.h"
#include "../../../include/storage.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <cjson/cJSON.h>   /* Phase 2 : parser JSON */

/* ============================================================================
 * Helpers internes
 * ============================================================================ */

static char *dup_json_string(cJSON *item) {
    if (!item || !cJSON_IsString(item)) return NULL;
    return strdup(item->valuestring);
}

static char *serialize_json(cJSON *item) {
    if (!item) return NULL;
    return cJSON_PrintUnformatted(item);
}

/* ============================================================================
 * parse_check_type() et parse_severity() déjà corrects
 * ============================================================================ */

static CheckType parse_check_type(const char *str) {
    if (!str) return CHECK_UNKNOWN;
    if (strcmp(str, "section_exists")  == 0) return CHECK_SECTION_EXISTS;
    if (strcmp(str, "section_order")   == 0) return CHECK_SECTION_ORDER;
    if (strcmp(str, "word_count_min")  == 0) return CHECK_WORD_COUNT_MIN;
    if (strcmp(str, "word_count_max")  == 0) return CHECK_WORD_COUNT_MAX;
    if (strcmp(str, "regex_forbidden") == 0) return CHECK_REGEX_FORBIDDEN;
    if (strcmp(str, "regex_required")  == 0) return CHECK_REGEX_REQUIRED;
    if (strcmp(str, "heading_format")  == 0) return CHECK_HEADING_FORMAT;
    if (strcmp(str, "citation_present")== 0) return CHECK_CITATION_PRESENT;
    if (strcmp(str, "llm_semantic")    == 0) return CHECK_LLM_SEMANTIC;
    return CHECK_UNKNOWN;
}

static Severity parse_severity(const char *str) {
    if (!str) return SEVERITY_WARNING;
    if (strcmp(str, "error")   == 0) return SEVERITY_ERROR;
    if (strcmp(str, "warning") == 0) return SEVERITY_WARNING;
    if (strcmp(str, "info")    == 0) return SEVERITY_INFO;
    return SEVERITY_WARNING;
}

/* ============================================================================
 * parse_rule_object() — cœur du parser
 * ============================================================================ */

static void parse_rule_object(cJSON *obj, Rule *out) {
    if (!obj || !out) return;

    /* id */
    cJSON *id = cJSON_GetObjectItem(obj, "id");
    if (cJSON_IsString(id))
        strncpy(out->id, id->valuestring, RULES_MAX_ID_LEN - 1);

    /* description */
    cJSON *desc = cJSON_GetObjectItem(obj, "description");
    if (cJSON_IsString(desc))
        strncpy(out->description, desc->valuestring, RULES_MAX_DESC_LEN - 1);

    /* category */
    cJSON *cat = cJSON_GetObjectItem(obj, "category");
    if (cJSON_IsString(cat))
        strncpy(out->category, cat->valuestring, RULES_MAX_CATEGORY_LEN - 1);

    /* check_type */
    cJSON *ctype = cJSON_GetObjectItem(obj, "check_type");
    if (cJSON_IsString(ctype))
        out->check_type = parse_check_type(ctype->valuestring);

    /* severity */
    cJSON *sev = cJSON_GetObjectItem(obj, "severity");
    if (cJSON_IsString(sev))
        out->severity = parse_severity(sev->valuestring);

    /* parameter : string / object / array */
    cJSON *param = cJSON_GetObjectItem(obj, "parameter");
    if (param) {
        if (cJSON_IsString(param)) {
            strncpy(out->parameter, param->valuestring, RULES_MAX_PARAM_LEN - 1);
        } else {
            char *json = serialize_json(param);
            if (json) {
                strncpy(out->parameter, json, RULES_MAX_PARAM_LEN - 1);
                free(json);
            }
        }
    }

    /* flags */
    cJSON *flags = cJSON_GetObjectItem(obj, "flags");
    if (flags && cJSON_IsObject(flags)) {
        cJSON *ci = cJSON_GetObjectItem(flags, "case_insensitive");
        out->flags.case_insensitive = cJSON_IsBool(ci) ? cJSON_IsTrue(ci) : false;
    }

    /* target_section */
    cJSON *ts = cJSON_GetObjectItem(obj, "target_section");
    if (cJSON_IsString(ts))
        strncpy(out->target_section, ts->valuestring, RULES_MAX_SECTION_LEN - 1);
}

/* ============================================================================
 * ruleset_load_from_file() — fonction principale
 * ============================================================================ */

RuleSet *ruleset_load_from_file(const char *filepath) {
    if (!filepath) return NULL;

    size_t len = 0;
    char *json_text = storage_read_file(filepath, &len);
    if (!json_text) {
        fprintf(stderr, "[ERROR] Impossible de lire %s\n", filepath);
        return NULL;
    }

    cJSON *root = cJSON_Parse(json_text);
    free(json_text);

    if (!root) {
        fprintf(stderr, "[ERROR] JSON invalide dans %s\n", filepath);
        return NULL;
    }

    RuleSet *set = calloc(1, sizeof(RuleSet));
    if (!set) {
        cJSON_Delete(root);
        return NULL;
    }

    /* meta */
    cJSON *meta = cJSON_GetObjectItem(root, "meta");
    if (meta && cJSON_IsObject(meta)) {
        cJSON *dt = cJSON_GetObjectItem(meta, "document_type");
        if (cJSON_IsString(dt))
            strncpy(set->meta.document_type, dt->valuestring, 63);

        cJSON *ver = cJSON_GetObjectItem(meta, "version");
        if (cJSON_IsString(ver))
            strncpy(set->meta.version, ver->valuestring, 31);

        cJSON *auth = cJSON_GetObjectItem(meta, "author");
        if (cJSON_IsString(auth))
            strncpy(set->meta.author, auth->valuestring, 63);
    }

    /* rules */
    cJSON *rules = cJSON_GetObjectItem(root, "rules");
    if (rules && cJSON_IsArray(rules)) {
        int count = cJSON_GetArraySize(rules);
        set->rule_count = (count > RULES_MAX_RULES) ? RULES_MAX_RULES : count;

        for (int i = 0; i < set->rule_count; i++) {
            cJSON *obj = cJSON_GetArrayItem(rules, i);
            parse_rule_object(obj, &set->rules[i]);
        }
    }

    cJSON_Delete(root);
    return set;
}
