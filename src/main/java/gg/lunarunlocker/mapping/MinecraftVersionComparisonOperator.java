package gg.lunarunlocker.mapping;


public enum MinecraftVersionComparisonOperator {
    EQUAL {
        @Override
        public boolean N(int current, int version) {
            return current == version;
        }
    },
    NOT_EQUAL {
        @Override
        public boolean N(int current, int version) {
            return current != version;
        }
    },
    GREATER_THAN {
        @Override
        public boolean N(int current, int version) {
            return current > version;
        }
    },
    GREATHER_THAN_OR_EQUAL {
        @Override
        public boolean N(int current, int version) {
            return current >= version;
        }
    },
    LESS_THAN {
        @Override
        public boolean N(int current, int version) {
            return current < version;
        }
    },
    LESS_THAN_OR_EQUAL {
        @Override
        public boolean N(int current, int version) {
            return current <= version;
        }
    },
    EQUALS {
        @Override
        public boolean N(int current, int version) {
            return current == version;
        }
    };

    public abstract boolean N(int current, int version);
}
