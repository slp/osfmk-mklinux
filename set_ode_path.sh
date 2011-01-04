:

# path to mig, makeboot, etc.
PATH=/usr/src/DR3/osfmk/tools/ppc/ppc_linux/hostbin:$PATH

# The concept here is that if you are in the ODE environment, then
# you have the ODE tools earlier in your path.  You will then get the
# ODE version of "make" instead of the regular version installed on the
# system.
# Otherwise, you want the the ode tools last in your path, or at least
# after the regular "make".

if [ -z "${WORKON}" ]; then
	PATH=$PATH:/usr/src/ode-bin/ppc_linux/bin
else
	PATH=/usr/src/ode-bin/ppc_linux/bin:$PATH
fi
export PATH
