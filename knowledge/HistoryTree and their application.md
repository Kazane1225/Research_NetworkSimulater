---

# Paper Summary: History Trees and Their Applications

## 1. Abstract & Introduction

* 
**Background**: In the theoretical study of distributed communication networks, an anonymous network is one where agents initially lack unique identifiers and are indistinguishable. They can only be told apart based on the network's layout or by receiving different sets of messages from neighboring agents.


* 
**History Trees**: This discrete structure naturally models how anonymous agents become distinguishable by systematically organizing temporal information. It has been instrumental in developing optimal deterministic algorithms for anonymous, dynamically evolving networks.


* 
**Core Contribution**: The note provides an accessible introduction to history trees, compares them with traditional static network structures (like views and graph fibrations), reviews recent advancements in dynamic networks, and outlines several open problems.



---

## 2. Basic Structure and Algorithms

* 
**Network Model**: This section focuses on anonymous networks operating in synchronous steps, modeled as undirected dynamic multigraphs with no port awareness.


* **The Structure of History Trees**:
* 
**Levels ($L_t$)**: The infinite tree is subdivided into levels, where nodes in level $L_t$ represent the equivalence classes of agents that are indistinguishable at time $t$.


* 
**Anonymity $a(v)$**: The number of agents in the class represented by a node $v$. The root $r$ represents all agents in the network. The children of a node $v \in L_{t-1}$ represent a partition of the agents represented by $v$, meaning $a(v) = \sum_{i=1}^{k} a(v_i)$.


* 
**Red Edges ($R_t$)**: Directed red edges represent messages sent and received at step $t$. A directed red edge $(v, u)$ with multiplicity $m$ indicates that each agent represented by node $u$ receives exactly $m$ identical messages from agents represented by $v$.




* **Local Views**:
* Assuming unbounded memory and message sizes, agents can locally construct portions of the history tree called views via an iterative match-and-merge process.


* After $t$ steps, the size of a view is $O(tn^2 \log M)$ bits, where $n$ is the total number of agents and $M$ is the maximum number of messages sent by any agent in a single step.





### Comparison with Related Structures for Static Networks

* **Yamashita-Kameda's Views & Boldi-Vigna's Minimum Bases**:
* These frameworks were developed for networks with unchanging topologies (static networks).


* At stabilization in a static network, the directed graph of the red edges between non-branching levels is isomorphic to the Boldi-Vigna minimum base $\hat{G}$.


* While Yamashita-Kameda views ($\mathcal{T}_G^k(p)$) contain the same information as history tree views ($\mathcal{V}_G^k(p)$), history trees inherently include a temporal dimension. This features timing information on when agents become distinguishable, making them uniquely suitable for dynamic networks.





### Basic Applications

* **Average Consensus & Counting**:
* If a level $L_i$ is non-branching and the network is connected, the bidirectional nature of communication links allows agents to infer the ratio of the anonymities involved using the red edge multiplicities: $m_{v,u'}a(u) = m_{u,v'}a(v)$.


* If the network contains a unique distinguished agent (leader), the **Counting** problem (computing the total number of agents $n$) can be solved.


* Information takes fewer than $n$ steps to travel through a connected network (dynamic diameter $d \le n-1$). At time $n+d-1 \le 2n-2$, all agents have enough information in their views to perform correct local computations.


* This yields an Average Consensus and a stabilizing Counting algorithm that stabilizes in $2n-2$ steps, which is proven to be optimal via lower-bound network examples.





---

## 3. Variations and Extensions

The paper expands the history tree framework to more complex network environments and algorithmic constraints.

| Extension / Scenario | Core Approach & Mechanisms | Running Time / Complexity |
| --- | --- | --- |
| <br>**Leader Election** 

 | Possible if and only if the tree contains a node of anonymity 1. Agents deterministically pick the node of smallest anonymity in the first non-branching level after $L_{\lfloor t/2 \rfloor}$.

 | Runs indefinitely without a certificate; if $n$ is known, terminates by time $t+2n-2$.

 |
| <br>**Terminating Counting** 

 | Computes exact network size and terminates using a correctness certificate based on "guessers" and "heavy nodes" to verify if guesses on anonymities are correct.

 | Terminates in $3n-2$ steps in the worst case.

 |
| <br>**Multi-Leader Counting** 

 | Handles a known number $l > 1$ of indistinguishable leaders. The solution involves subdividing the history tree into $l$ intervals.

 | Total running time is roughly $(l^2 + l + 1)n$ steps.

 |
| <br>**Directed Networks** 

 | Assumes a **late outdegree awareness** model. Outdegrees are attached to black edges, forming a homogeneous system represented by an irreducible matrix $A$. The system is solved via the Perron-Frobenius theorem.

 | Stabilization takes $2n-2$ steps (optimal). Termination takes $2^{O(n \log n)}$ steps.

 |
| <br>**Disconnected Networks** 

 | Evaluated using $\tau$-union-connected networks, where a connected communication round occurs every block of $\tau$ steps. Agents accumulate messages and update views once every $\tau$ steps.

 | Causes a worst-case optimal slowdown by a factor of $\tau$.

 |
| <br>**Semi-Synchronous Networks** 

 | Agents can be unpredictably inactive and cannot count steps, leading to unevenly sized views. Dummy nodes are added to create an "equalized view".

 | Reduced to the equivalent synchronous $\tau$-union-connected model.

 |
| <br>**Asynchronous Networks** 

 | Unpredictable message delays. Discretized time rounds are evaluated. Agents expand views upon sending and seek non-branching intervals constituting a round.

 | Total stabilization time is $2n-2$ rounds (worst-case optimal).

 |
| <br>**Port Awareness** 

 | <br>**Output port awareness** allows agents to tag outgoing messages, breaking network symmetry. Red paths originating from the leader have an anonymity of 1, making counting simpler.

 | Terminating Counting algorithm finishes in $2n-1$ steps.

 |
| <br>**Varying Inputs** 

 | Supports attaching changing inputs directly to nodes, converting stabilizing algorithms into streaming algorithms.

 | Adaptively returns correct outputs with an amortized delay of $n-1$ steps.

 |
| <br>**Self-Stabilization** 

 | Returns the correct output regardless of the initial state. Agents deliberately "forget" old information by deleting level $L_0$ and merging equivalent nodes.

 | If $n$ is known: purges levels exceeding $2n-2$ steps. If $n$ is unknown: toggles flags to reach equal height in $O(n)$ steps.

 |
| <br>**Finite-State Stabilization** 

 | Limits memory to a finite amount as a function of $n$. Neighboring agents skip merging and remain inactive if their shallowest suitable levels are isomorphic, saving state space.

 | Introduces a time overhead of $O(n^2)$ steps.

 |
| <br>**Memoryless Computation** 

 | Agents' states are completely reset at every communication step. A protocol must enable them to construct coherent views of a related history tree.

 | <br>*Open Problem* 

 |
| <br>**Congested Networks** 

 | Bandwidth is limited to $O(\log n)$ bits, so entire views cannot be sent at once. Logarithmic-sized unique labels are instead broadcasted one level at a time.

 | <br>$d$ known: $O(dn^2)$ steps. $d$ unknown (with 1 leader): $O(n^3)$ steps.

 |

---

## 4. Key Open Problems

The paper highlights 13 open problems to guide future research on history trees. Some of the most notable include:

* 
**Open Problem 1**: Let $G$ and $G'$ be two disjoint static networks of $n$ agents each. If an agent in $G$ and an agent in $G'$ have isomorphic views at time $t=n-1$, do they have isomorphic views at all times? 


* 
**Open Problem 2**: Can a Counting algorithm stabilize in $2n-3$ steps in all connected undirected dynamic networks with a unique leader? 


* 
**Open Problem 3**: Can a Counting algorithm terminate in $2n+O(1)$ steps in all connected undirected dynamic networks with a unique leader? 


* 
**Open Problem 5**: Can a Counting algorithm terminate in a polynomial number of steps in all strongly connected directed dynamic simple networks with (early or late) outdegree awareness and a unique leader? 


* 
**Open Problem 8**: Is there a universal finite-state stabilizing protocol for connected undirected dynamic networks with an overhead of $O(n)$ steps? 


* 
**Open Problem 10**: Under what assumptions is there a universal memoryless protocol for anonymous networks? 


* 
**Open Problem 12**: Can a Counting algorithm terminate in $O(n^2)$ steps in all congested dynamic networks with a unique leader?